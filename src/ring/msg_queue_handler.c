#include "mem/mem_debug.h"
#include "common_macro.h"
#include "ring/msg_queue_handler.h"
#include "ring/msg_queue.h"
#include "thread/portable_thread.h"
#define LOG_TAG "MSG_Q_HDL"
#include "log/slog.h"
#include <string.h>
#include <malloc.h>

#define Q_LOGE(fmt,...)    SLOGE(LOG_TAG, fmt, ##__VA_ARGS__)
#ifdef _DEBUG
#define Q_LOGV(fmt,...)    SLOGV(LOG_TAG, fmt, ##__VA_ARGS__)
#define Q_LOGD(fmt,...)    SLOGD(LOG_TAG, fmt, ##__VA_ARGS__)
#define Q_LOGI(fmt,...)    SLOGI(LOG_TAG, fmt, ##__VA_ARGS__)
#define Q_LOGW(fmt,...)    SLOGW(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define Q_LOGV(fmt,...)
#define Q_LOGD(fmt,...)
#define Q_LOGI(fmt,...)
#define Q_LOGW(fmt,...)
// #define Q_LOGE(fmt,...)
#endif // _DEBUG

/* ========== 大消息封装实现（使用保留 what 值） ========== */
/* 使用头文件里的 MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT (INT32_MIN) 作为内部标识。 */

/* 大消息引用结构: 存放真正数据内存指针与大小以及原始 what */
typedef struct
{
	int orig_what;      /* 原始消息 what 值 */
	uint32_t data_size; /* 原始对象数据大小 */
	void *data_ptr;     /* 指向真正的数据内存 */
} big_msg_obj_ref_ex_t;

/* 大消息封装信封结构: 包含消息头和大消息引用, 这个是push到队列的数据结构 */
typedef struct
{
	queue_msg_header_t header;
	big_msg_obj_ref_ex_t ref;
} big_msg_envelope_t;

/* 消息队列上下文 */
struct msg_queue_handler_s
{
	msg_queue_handler_init_param_t param;
	msg_queue msg_queue_p;

	volatile bool flag2exit;               /* 普通销毁标志 */
	volatile bool flag_destroy_gracefully; /* 优雅销毁: 退出阶段仍交给用户处理 */
	portable_thread_t thread_handler;
	portable_sem_t semaphore;
};

#define MSG_Q_HANDLER_SIZE (sizeof(struct msg_queue_handler_s))

static size_t pri_roundup_power2(size_t n)
{
	if (n & (n - 1))
	{
		size_t counter = 0;
		while (n >>= 1)
		{
			++counter;
		}
		n = (2ULL << counter);
	}
	return n;
}

static void pri_notify_status_changed(msg_queue_handler handler, msg_q_handler_status_e status)
{
	if (NULL == handler->param.callback.fn_on_status_changed)
	{
		return;
	}
	handler->param.callback.fn_on_status_changed(status, handler->param.callback.user_data);
}

static void *thread_worker_handle_msg(void *thread_context)
{
	msg_queue_handler handler = (msg_queue_handler)thread_context;
	Q_LOGI(" (%s:%d) thread(%lu) started...", __func__, __LINE__, GETTID());
	pri_notify_status_changed(handler, MSG_Q_HANDLER_STATUS_READY_TO_GO);

	size_t cur_msg_buf_size = 4096U;
	char *poped_msg_buf = (char *)malloc(cur_msg_buf_size);
	if (!poped_msg_buf)
	{
		handler->flag2exit = true;
		Q_LOGE("can't malloc(%zu) on %s:%d, now thread exit...", cur_msg_buf_size, __func__, __LINE__);
		pri_notify_status_changed(handler, MSG_Q_HANDLER_STATUS_ABOUT_TO_STOP);
		return NULL;
	}

	int user_handle_ret = 0;
	msg_q_code_e last_status = MSG_Q_CODE_SUCCESS;
	for (;;)
	{
		if (!handler->flag2exit && (MSG_Q_CODE_SUCCESS == last_status || MSG_Q_CODE_EMPTY == last_status))
		{
			/* 正常工作阶段等待信号 */
			portable_sem_wait(&handler->semaphore);
		}

		uint32_t popped_msg_size = (uint32_t)cur_msg_buf_size;
		last_status = msg_queue_pop(handler->msg_queue_p, poped_msg_buf, &popped_msg_size);
		if (MSG_Q_CODE_SUCCESS != last_status)
		{
			if (MSG_Q_CODE_EMPTY == last_status)
			{
				if (handler->flag2exit)
				{
					/* 退出阶段且队列空 -> 结束 */
					break;
				}
				continue; /* 正常阶段无消息继续等待 */
			}
			if (MSG_Q_CODE_BUF_NOT_ENOUGH == last_status)
			{
				free(poped_msg_buf);
				size_t expect_buf_size = pri_roundup_power2(popped_msg_size);
				poped_msg_buf = (char *)malloc(expect_buf_size);
				if (!poped_msg_buf)
				{
					Q_LOGE("can't malloc(%zu) on %s:%d, now exit...", expect_buf_size, __func__, __LINE__);
					break;
				}
				cur_msg_buf_size = expect_buf_size;
				continue; /* 重新尝试 pop */
			}
			if (MSG_Q_CODE_AGAIN == last_status)
			{
				usleep(1000); /* 等 1ms 再试 */
				continue;
			}
			/* 其它错误直接继续或根据需要退出，这里继续 */
			continue;
		}

		queue_msg_t *msg = (queue_msg_t *)poped_msg_buf;
		bool is_big_msg = (MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT == msg->what);
		if (is_big_msg)
		{
			/* 校验结构最小字段尺寸: orig_what + data_size + data_ptr */
			if (msg->obj_len < (int)(sizeof(int) + sizeof(uint32_t) + sizeof(void *)))
			{
				Q_LOGE("invalid big indirect msg format(obj_len=%d) -- discard", msg->obj_len);
				continue;
			}
		}

		if (!handler->flag2exit || handler->flag_destroy_gracefully)
		{
			if (is_big_msg)
			{
				big_msg_obj_ref_ex_t *ref = (big_msg_obj_ref_ex_t *)msg->obj;
				queue_msg_t *user_msg = (queue_msg_t *)malloc(sizeof(queue_msg_t) + ref->data_size);
				if (!user_msg)
				{
					Q_LOGE("failed malloc user_msg for big indirect(size=%u)", ref->data_size);
					if (ref->data_ptr)
					{
						free(ref->data_ptr);
					}
					handler->flag2exit = true;
					continue;
				}
				user_msg->what = ref->orig_what;
				user_msg->arg1 = msg->arg1;
				user_msg->arg2 = msg->arg2;
				user_msg->obj_len = (int)ref->data_size;
				memcpy(user_msg->obj, ref->data_ptr, ref->data_size);
				user_handle_ret = handler->param.callback.fn_handle_msg(user_msg, handler->param.callback.user_data);
				free(ref->data_ptr);
				free(user_msg);
				if (0 != user_handle_ret)
				{
					Q_LOGE("error(%d) on user process big msg, switch to draining mode...", user_handle_ret);
					handler->flag2exit = true;
					continue;
				}
			}
			else
			{
				if (0 != (user_handle_ret = handler->param.callback.fn_handle_msg(msg, handler->param.callback.user_data)))
				{
					Q_LOGE("error(%d) on user process msg, switch to draining mode...", user_handle_ret);
					handler->flag2exit = true;
					continue;
				}
			}
		}
		else
		{
			/* 退出阶段: 优雅销毁之外仅释放大消息内存 */
			if (is_big_msg)
			{
				big_msg_obj_ref_ex_t *ref = (big_msg_obj_ref_ex_t *)msg->obj;
				if (ref->data_ptr)
				{
					free(ref->data_ptr);
				}
			}
		}
	} // END of for(;;)

	handler->flag2exit = true; // <-- mark thread exited
	if (poped_msg_buf)
	{
		free(poped_msg_buf);
	}
	Q_LOGI(" (%s:%d) thread(%lu) exited...", __func__, __LINE__, GETTID());
	pri_notify_status_changed(handler, MSG_Q_HANDLER_STATUS_ABOUT_TO_STOP);
	return NULL;
}

msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size,
										   __in msg_queue_handler_init_param_t *param_p)
{
	Q_LOGD("create msg_queue_handler. queue_buf_size=%u", queue_buf_size);
	if (queue_buf_size < 4U || !param_p || NULL == param_p->callback.fn_handle_msg)
	{
		Q_LOGE("queue_buf_size(%u) shouldn't smaller than 4, and param_p(%p) with callback shouldn't be NULL",
			 queue_buf_size, (void *)param_p);
		return NULL;
	}
	msg_queue_handler handler = (msg_queue_handler)calloc(1U, MSG_Q_HANDLER_SIZE);
	if (!handler)
	{
		Q_LOGE("failed alloc queue_handler memory. expect handler_size=%u", (uint32_t)MSG_Q_HANDLER_SIZE);
		return NULL;
	}
	handler->flag2exit = false;
	handler->flag_destroy_gracefully = false;
	memcpy(&(handler->param), param_p, sizeof(msg_queue_handler_init_param_t));
	if (0 != portable_sem_init(&(handler->semaphore), 0, 0) || !handler->semaphore ||
		NULL == (handler->msg_queue_p = msg_queue_create(queue_buf_size)) ||
		0 != portable_thread_create(&(handler->thread_handler), NULL, thread_worker_handle_msg, handler))
	{
		Q_LOGE("error on create thread or queue! expect queue_buf_size=%u", queue_buf_size);
		/* P1-6: portable_thread_create 失败时 thread_handler 状态未定义(POSIX/Win 实现相关),
		 * 显式置 0, 避免 destroy 路径 if(thread_handler) join 垃圾句柄导致 UB. */
		handler->thread_handler = 0;
		msg_queue_handler_destroy(&handler, MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY);
		handler = NULL;
	}
	return handler;
}

msg_q_code_e msg_queue_handler_push(__in msg_queue_handler handler, __in queue_msg_t *msg_p)
{
	if (!handler || !handler->msg_queue_p)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	if (!msg_p)
	{
		return MSG_Q_CODE_INVALID_MSG;
	}
	/* H-2 修复: 校验 obj_len 非负。
	 * 虽然头文件契约未明确, 但负长度无语义，且会导致两处问题:
	 * 1. line 274: (uint32_t)(sizeof + obj_len) 若 obj_len 为负, int 加法后转 uint32_t 仍正确
	 * 2. line 292: malloc(msg_p->obj_len) 直接传 int, 负数提升为巨型 size_t 导致分配失败或 OOM
	 * 在入口统一拒绝负数。 */
	if (msg_p->obj_len < 0)
	{
		Q_LOGE("invalid obj_len: %d (must be non-negative)", msg_p->obj_len);
		return MSG_Q_CODE_INVALID_MSG;
	}
	if (handler->flag2exit)
	{
		return MSG_Q_CODE_GENERIC_FAIL;
	}

	msg_q_code_e push_status = MSG_Q_CODE_GENERIC_FAIL;
	do
	{

		/* 1. 普通消息: 直接拷贝入环形缓冲区 */
		if (handler->param.cfg.threshold_mem_size_for_alloc_obj < 1U ||
			msg_p->obj_len < (int)handler->param.cfg.threshold_mem_size_for_alloc_obj)
		{
			push_status = msg_queue_push(handler->msg_queue_p, msg_p, (uint32_t)(sizeof(queue_msg_t) + msg_p->obj_len));
			break;
		}

		/* 2. 超大消息走引用封装路径 */
		/* 超大消息: 检查 what 是否为保留值 */
		if (MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT == msg_p->what)
		{
			Q_LOGE("push failed: 'what' uses reserved value MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT(%d)", MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT);
			push_status = MSG_Q_CODE_INVALID_MSG;
			break;
		}
		if (msg_queue_available_push_bytes(handler->msg_queue_p) < sizeof(big_msg_envelope_t))
		{
			push_status = MSG_Q_CODE_FULL;
			break;
		}

		void *ext_mem = malloc(msg_p->obj_len);
		if (!ext_mem)
		{
			push_status = MSG_Q_CODE_FAILED_ON_ALLOC;
			break;
		}
		memcpy(ext_mem, msg_p->obj, msg_p->obj_len);
		/* 构造连续内存: queue_msg_t 头部 + big_msg_obj_ref_ex_t 数据 */
		big_msg_envelope_t envelope;
		envelope.header.what = MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT; /* 使用保留 what 标识大消息 */
		envelope.header.arg1 = msg_p->arg1;
		envelope.header.arg2 = msg_p->arg2;
		envelope.header.obj_len = (int)sizeof(big_msg_obj_ref_ex_t);
		envelope.ref.orig_what = msg_p->what;
		envelope.ref.data_size = (uint32_t)msg_p->obj_len;
		envelope.ref.data_ptr = ext_mem;
		if (MSG_Q_CODE_SUCCESS !=
			(push_status = msg_queue_push(handler->msg_queue_p,
										  &envelope, (uint32_t)sizeof(big_msg_envelope_t))))
		{
			free(ext_mem); /* push 失败则释放申请的外部内存 */
		}
	} while (0);

	if (MSG_Q_CODE_SUCCESS == push_status)
	{
		portable_sem_post(&(handler->semaphore));
	}
	return push_status;
}

extern inline uint32_t msg_queue_handler_available_push_bytes(__in msg_queue_handler handler)
{
	return msg_queue_available_push_bytes(handler->msg_queue_p);
}

extern inline uint32_t msg_queue_handler_available_pop_bytes(__in msg_queue_handler handler)
{
	return msg_queue_available_pop_bytes(handler->msg_queue_p);
}

void msg_queue_handler_destroy(__inout msg_queue_handler *handler_p, __in int flags)
{
	if (!handler_p || !(*handler_p))
	{
		return;
	}
	msg_queue_handler handler = *handler_p;
	handler->flag2exit = true;
	handler->flag_destroy_gracefully = (0 != (flags & MSG_Q_HANDLER_DESTROY_FLAGS_GRACEFULLY)) ? true : false;
	/* P2-8: 循环 post 信号量, 覆盖 worker 在 AGAIN 路径(usleep 1ms + continue)的临界窗口.
	 * 修复前: 单次 post 在某些时序下被 worker 重试循环消费, 后续 sem_wait 永久阻塞 -> destroy join 卡死.
	 * 多次 post 是安全的: 信号量计数累加,不会有副作用. */
	if (handler->semaphore)
	{
		for (int i = 0; i < 8; ++i)
		{
			portable_sem_post(&(handler->semaphore));
		}
	}
	if (handler->thread_handler && 0 != portable_thread_join(handler->thread_handler, NULL))
	{
		Q_LOGE("error on join handle_msg_thread(graceful=%d)", handler->flag_destroy_gracefully);
	}
	if (handler->semaphore)
	{
		portable_sem_destroy(&(handler->semaphore));
	}
	if (handler->msg_queue_p)
	{
		msg_queue_destroy(&(handler->msg_queue_p));
	}
	memset(handler, 0, MSG_Q_HANDLER_SIZE);
	free(handler);
	*handler_p = NULL;
}
