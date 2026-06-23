#include "mem/mem_debug.h"
#include "ring/fixed_msg_queue.h"
#include "ring/fixed_msg_queue_handler.h"
#include "thread/portable_thread.h"
#define LOG_TAG "FIXED_Q_HDL"
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

typedef struct
{
	size_t token;
	fixed_msg_t out_msg;
} fixed_handler_msg_t;

struct _fixed_msg_queue_handler_s
{
	fixed_msg_queue_handler_init_param_t param;
	fixed_msg_queue msg_queue_p;

	/* 标记进入销毁阶段, 不再允许 push */
	volatile bool destroying;
	volatile bool graceful_destroy; /* true=优雅销毁 */
	portable_thread_t thread_handler;
	portable_sem_t semaphore;

	fixed_handler_msg_t msg_send_cache;
	size_t token_counter;
	volatile size_t min_valid_token;
};

static void pri_notify_status_changed(fixed_msg_queue_handler handler, msg_q_handler_status_e status)
{
	if (NULL == handler->param.callback.fn_on_status_changed)
	{
		return;
	}
	handler->param.callback.fn_on_status_changed(status, handler->param.callback.user_data);
}

/* 统一处理已弹出的消息；返回 true 表示继续处理，false 表示需要退出线程 */
static bool pri_handle_popped_msg(fixed_msg_queue_handler handler,
								  fixed_handler_msg_t *handler_msg,
								  bool draining)
{
	if (handler_msg->token < handler->min_valid_token)
	{
		Q_LOGW("abandon msg%s: token=%zu, min_valid_token=%zu",
			 draining ? " (graceful drain)" : "",
			 handler_msg->token, handler->min_valid_token);
		return true; /* 丢弃后继续 */
	}
	int user_ret = handler->param.callback.fn_handle_msg(&handler_msg->out_msg, handler->param.callback.user_data);
	if (0 != user_ret)
	{
		Q_LOGE("error(%d) on user process msg%s, now exit", user_ret,
			 draining ? " (graceful drain)" : "");
		return false; /* 终止线程 */
	}
	return true;
}

static void *thread_worker_handle_msg(void *thread_context)
{
	fixed_msg_queue_handler handler = (fixed_msg_queue_handler)thread_context;
	Q_LOGI(" (%s:%d) thread(%lu) started...", __func__, __LINE__, GETTID());
	pri_notify_status_changed(handler, MSG_Q_HANDLER_STATUS_READY_TO_GO);

	fixed_handler_msg_t handler_msg = {0, {0}};
	for (;;)
	{
		if (!handler->destroying)
		{
			portable_sem_wait(&handler->semaphore);
		}
		if (handler->destroying)
		{
			if (handler->graceful_destroy)
			{
				/* 优雅销毁：尽量消费剩余消息 */
				while (fixed_msg_queue_pop(handler->msg_queue_p, &handler_msg))
				{
					if (!pri_handle_popped_msg(handler, &handler_msg, true))
					{
						goto L_THREAD_EXIT;
					}
				}
			}
			goto L_THREAD_EXIT; /* 普通销毁直接退出 */
		}

		if (!fixed_msg_queue_pop(handler->msg_queue_p, &handler_msg))
		{
			continue; /* 空队列或虚假唤醒 */
		}
		if (!pri_handle_popped_msg(handler, &handler_msg, false))
		{
			break; /* user 要求退出 */
		}
	}

L_THREAD_EXIT:
	handler->destroying = true; // mark thread exit
	Q_LOGI(" (%s:%d) thread(%lu) exited...", __func__, __LINE__, GETTID());
	pri_notify_status_changed(handler, MSG_Q_HANDLER_STATUS_ABOUT_TO_STOP);
	return NULL;
}

fixed_msg_queue_handler fixed_msg_queue_handler_create(__in uint32_t max_msg_capacity,
													   __in fixed_msg_queue_handler_init_param_t *init_param_p)
{
	if (max_msg_capacity < 2U || NULL == init_param_p || NULL == init_param_p->callback.fn_handle_msg)
	{
		Q_LOGD("invalid param. max_msg_capacity=%u", max_msg_capacity);
		return NULL;
	}
	Q_LOGD("create fixed_queue_handler. max_msg_capacity=%u", max_msg_capacity);
	fixed_msg_queue_handler handler = (fixed_msg_queue_handler)calloc(1, sizeof(struct _fixed_msg_queue_handler_s));
	if (!handler)
	{
		return NULL;
	}
	handler->destroying = false;
	handler->graceful_destroy = false;
	memcpy(&handler->param, init_param_p, sizeof(fixed_msg_queue_handler_init_param_t));
	handler->token_counter = 0;
	handler->min_valid_token = 0;
	if (0 != portable_sem_init(&(handler->semaphore), 0, 0) || NULL == handler->semaphore ||
		NULL == (handler->msg_queue_p = fixed_msg_queue_create(sizeof(fixed_handler_msg_t), max_msg_capacity)) ||
		0 != portable_thread_create(&(handler->thread_handler), NULL, thread_worker_handle_msg, handler))
	{
		Q_LOGE("error on create thread or queue!");
		fixed_msg_queue_handler_destroy(&handler, FIXED_MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY);
		handler = NULL;
	}
	return handler;
}

msg_q_code_e fixed_msg_queue_handler_push(__in fixed_msg_queue_handler handler, __in fixed_msg_t *msg_p)
{
	if (!handler || !(handler->msg_queue_p) || handler->destroying)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	// um... here we should lock it on multi-thread, let user do it
	fixed_handler_msg_t *handler_msg_p = &(handler->msg_send_cache);
	handler_msg_p->out_msg = *msg_p; // perform copy inside
	handler_msg_p->token = handler->token_counter;

	if (!fixed_msg_queue_push(handler->msg_queue_p, handler_msg_p))
	{
		// Q_LOGE("send msg to queue handled failed. queue is full");
		return MSG_Q_CODE_FULL;
	}

	++handler->token_counter;
	portable_sem_post(&(handler->semaphore));
	return MSG_Q_CODE_SUCCESS;
}

extern inline uint32_t fixed_msg_queue_handler_available_push_amount(__in fixed_msg_queue_handler handler)
{
	return fixed_msg_queue_available_push_amount(handler->msg_queue_p);
}

extern inline uint32_t fixed_msg_queue_handler_available_pop_amount(__in fixed_msg_queue_handler handler)
{
	return fixed_msg_queue_available_pop_amount(handler->msg_queue_p);
}

extern inline bool fixed_msg_queue_handler_is_empty(__in fixed_msg_queue_handler handler)
{
	return 0 == fixed_msg_queue_handler_available_pop_amount(handler);
}

extern inline bool fixed_msg_queue_handler_is_full(__in fixed_msg_queue_handler handler)
{
	return 0 == fixed_msg_queue_handler_available_push_amount(handler);
}

extern inline void fixed_msg_queue_handler_clear(__in fixed_msg_queue_handler handler)
{
	handler->min_valid_token = handler->token_counter;
	Q_LOGD("fixed_msg_queue_handler_clear handler(%p) min_valid_token=%zu",
		 handler, handler->min_valid_token);
}

void fixed_msg_queue_handler_destroy(__inout fixed_msg_queue_handler *handler_p, __in int flags)
{
	if (!handler_p || !(*handler_p))
	{
		return;
	}
	fixed_msg_queue_handler handler = *handler_p;
	/* 标记销毁模式，由工作线程根据 graceful_destroy 决定是否 drain */
	handler->destroying = true;
	handler->graceful_destroy = (flags == FIXED_MSG_Q_HANDLER_DESTROY_FLAGS_GRACEFULLY);
	if (handler->semaphore)
	{
		portable_sem_post(&(handler->semaphore)); /* 唤醒一次即可 */
	}
	if (handler->thread_handler &&
		0 != portable_thread_join(handler->thread_handler, NULL))
	{
		Q_LOGE("error on join handle msg thread.");
	}
	if (handler->semaphore)
	{
		portable_sem_destroy(&(handler->semaphore));
	}
	if (handler->msg_queue_p)
	{
		fixed_msg_queue_destroy(&(handler->msg_queue_p));
	}
	memset(handler, 0, sizeof(struct _fixed_msg_queue_handler_s));
	free(handler);
	*handler_p = NULL;
}
