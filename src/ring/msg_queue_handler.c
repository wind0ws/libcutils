#include "mem/mem_debug.h"
#include "common_macro.h"
#include "ring/msg_queue_handler.h"
#include "ring/msg_queue.h"
#include "thread/portable_thread.h"
#define LOG_TAG "MSG_Q_HDL"
#include "log/slog.h"
#include <string.h>
#include <malloc.h>

struct _msg_queue_handler_s
{
	msg_queue_handler_init_param_t param;
	msg_queue msg_queue_p;

	volatile bool flag2exit;
	portable_thread_t thread_handler;
	portable_sem_t semaphore;
};

static size_t pri_roundup_power2(size_t n)
{
	if (n & (n - 1))
	{
		size_t counter = 0;
		while (n >>= 1)
		{
			++counter;
		}
		n = (2 << counter);
	}
	return n;
}

static void pri_notify_status(msg_queue_handler handler, msg_q_handler_status_e status)
{
	if (NULL == handler->param.notify_msg_handler_status)
	{
		return;
	}
	handler->param.notify_msg_handler_status(status, handler->param.user_data);
}

static void* thread_worker_handle_msg(void* thread_context)
{
	msg_queue_handler handler = (msg_queue_handler)thread_context;
	LOGI(" %s:%d thread(%lu) started...", __func__, __LINE__, GETTID());
	pri_notify_status(handler, MSG_Q_HANDLER_STATUS_STARTED);

	size_t cur_msg_buf_size = 4096U;
	char* poped_msg_buf = (char *)malloc(cur_msg_buf_size);
	if (!poped_msg_buf)
	{
		handler->flag2exit = true;
		LOGE("can't malloc(%zu) on %s:%d, now thread exit...", cur_msg_buf_size, __func__, __LINE__);
		pri_notify_status(handler, MSG_Q_HANDLER_STATUS_STOPPED);
		return NULL;
	}

	int user_handle_ret = 0;
	msg_q_code_e last_status = MSG_Q_CODE_SUCCESS;
	for(;;)
	{
		if (MSG_Q_CODE_SUCCESS == last_status || MSG_Q_CODE_EMPTY == last_status)
		{
			portable_sem_wait(&handler->semaphore);
		}
		if (handler->flag2exit)
		{
			break;
		}
		uint32_t popped_msg_size = (uint32_t)cur_msg_buf_size;
		last_status = msg_queue_pop(handler->msg_queue_p, poped_msg_buf, &popped_msg_size);
		if (MSG_Q_CODE_SUCCESS != last_status)
		{
			if (MSG_Q_CODE_BUF_NOT_ENOUGH == last_status)
			{
				free(poped_msg_buf);
				size_t expect_buf_size = pri_roundup_power2(popped_msg_size);
				poped_msg_buf = (char*)malloc(expect_buf_size);
				if (!poped_msg_buf)
				{
					LOGE("can't malloc(%zu) on %s:%d, now exit...", expect_buf_size, __func__, __LINE__);
					break;
				}
				cur_msg_buf_size = expect_buf_size;
			}
			else if (MSG_Q_CODE_AGAIN == last_status)
			{
				usleep(1000); // <-- just wait 1ms and retry
			}
			continue;
		}

		queue_msg_t* msg = (queue_msg_t*)poped_msg_buf;
		if (0 != (user_handle_ret = handler->param.process_msg(msg, handler->param.user_data)))
		{
			LOGE("error(%d) on user process msg, now exit...", user_handle_ret);
			break;
		}
	}

	handler->flag2exit = true; // <-- for mark thread not handle msg yet!
	if (poped_msg_buf)
	{
		free(poped_msg_buf);
	}
	LOGI(" %s:%d thread(%lu) exited...", __func__, __LINE__, GETTID());
	pri_notify_status(handler, MSG_Q_HANDLER_STATUS_STOPPED);
	return NULL;
}

msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size,
	__in msg_queue_handler_init_param_t* param_p)
{
	LOGD("create msg_queue_handler. queue_buf_size=%d", queue_buf_size);
	if (queue_buf_size < 4U || !param_p || NULL == param_p->process_msg)
	{
		LOGE("queue_buf_size(%u) shouldn't smaller than 4, and param_p(%p) with callback shouldn't be NULL", 
			queue_buf_size, (void *)param_p);
		return NULL;
	}
	msg_queue_handler handler = (msg_queue_handler)calloc(1U, sizeof(struct _msg_queue_handler_s));
	if (!handler)
	{
		LOGE("failed alloc queue_handler memory.");
		return NULL;
	}
	handler->flag2exit = false;
	memcpy(&(handler->param), param_p, sizeof(msg_queue_handler_init_param_t));
	portable_sem_init(&(handler->semaphore), 0, 0);
	handler->msg_queue_p = msg_queue_create(queue_buf_size);
	if (!handler->msg_queue_p ||
		0 != portable_thread_create(&(handler->thread_handler), NULL, thread_worker_handle_msg, handler))
	{
		LOGE("error on create thread or queue!");
		msg_queue_handler_destroy(&handler);
		handler = NULL;
	}
	return handler;
}

msg_q_code_e msg_queue_handler_push(__in msg_queue_handler handler, __in queue_msg_t* msg_p)
{
	if (!handler || !handler->msg_queue_p || handler->flag2exit)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	msg_q_code_e push_status = msg_queue_push(handler->msg_queue_p, msg_p, (sizeof(queue_msg_t) + msg_p->obj_len));
	if (push_status == MSG_Q_CODE_SUCCESS)
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

void msg_queue_handler_destroy(__inout msg_queue_handler* handler_p)
{
	if (!handler_p || !(*handler_p))
	{
		return;
	}
	msg_queue_handler handler = *handler_p;
	handler->flag2exit = true;
	//send a signal to make sure thread is not stuck at sem_wait()
	portable_sem_post(&(handler->semaphore));
	if (handler->thread_handler && 
		0 != portable_thread_join(handler->thread_handler, NULL))
	{
		LOGE("error on join handle_msg_thread");
	}
	portable_sem_destroy(&(handler->semaphore));
	if (handler->msg_queue_p)
	{
		msg_queue_destroy(&handler->msg_queue_p);
	}
	handler->msg_queue_p = NULL;
	free(handler);
	*handler_p = NULL;
}
