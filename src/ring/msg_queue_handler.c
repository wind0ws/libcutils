#include "common_macro.h"
#include "ring/msg_queue_handler.h"
#include "ring/msg_queue.h"
#include "log/slog.h"
#include <malloc.h>
#include "thread/thread_wrapper.h"

#define LOG_TAG "MSG_Q_HDL"

struct _msg_queue_handler_s
{
	msg_queue msg_queue_p;
	msg_handler_callback_t callback;
	void* callback_userdata;
	pthread_t thread_handler;
	sem_t semaphore;
	volatile bool flag_exit_thread;
};

static size_t roundup_power2(size_t n)
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

static void* thread_fun_handle_msg(void* thread_context)
{
	msg_queue_handler handler = (msg_queue_handler)thread_context;
	size_t cur_msg_buf_size = 4096U;
	char* poped_msg_buf = (char *)malloc(cur_msg_buf_size);
	if (!poped_msg_buf)
	{
		SLOGE(LOG_TAG, "can't malloc(%zu) on %s:%d, now thread exit...", cur_msg_buf_size, __func__, __LINE__);
		return NULL;
	}
	MSG_Q_CODE last_status = MSG_Q_CODE_SUCCESS;
	while (!handler->flag_exit_thread)
	{
		if (last_status == MSG_Q_CODE_SUCCESS || last_status == MSG_Q_CODE_EMPTY)
		{
			sem_wait(&handler->semaphore);
		}
		if (handler->flag_exit_thread)
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
				size_t expect_buf_size = roundup_power2(popped_msg_size);
				poped_msg_buf = (char*)malloc(expect_buf_size);
				if (!poped_msg_buf)
				{
					SLOGE(LOG_TAG, "can't malloc(%zu) on %s:%d", expect_buf_size, __func__, __LINE__);
					break;
				}
				cur_msg_buf_size = expect_buf_size;
			}
			continue;
		}

		queue_msg_t* msg = (queue_msg_t*)poped_msg_buf;
		handler->callback(msg, handler->callback_userdata);
	}

	if (poped_msg_buf)
	{
		free(poped_msg_buf);
	}
	return NULL;
}

msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size,
	__in msg_handler_callback_t callback, __in void* callback_userdata)
{
	SLOGD(LOG_TAG, "create msg_queue_handler. queue_buf_size=%d", queue_buf_size);
	msg_queue_handler handler = (msg_queue_handler)calloc(1, sizeof(struct _msg_queue_handler_s));
	if (!handler)
	{
		return NULL;
	}
	handler->flag_exit_thread = false;
	handler->callback = callback;
	handler->callback_userdata = callback_userdata;
	sem_init(&(handler->semaphore), 0, 0);
	if (pthread_create(&(handler->thread_handler), NULL, thread_fun_handle_msg, handler) == 0)
	{
		char thr_name[32] = { 0 };
		snprintf(thr_name, sizeof(thr_name), "q_hdl_%p", handler);
		pthread_set_name(handler->thread_handler, thr_name);
		handler->msg_queue_p = msg_queue_create(queue_buf_size);
	}
	else
	{
		SLOGE(LOG_TAG, "error on create pthread of queue handle msg");
		sem_destroy(&(handler->semaphore));
		free(handler);
		handler = NULL;
	}
	return handler;
}

MSG_Q_CODE msg_queue_handler_send(__in msg_queue_handler handler, __in queue_msg_t* msg_p)
{
	if (!handler || handler->msg_queue_p == NULL || handler->flag_exit_thread)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	MSG_Q_CODE push_status = msg_queue_push(handler->msg_queue_p, msg_p, (sizeof(queue_msg_t) + msg_p->obj_len));
	if (push_status == MSG_Q_CODE_SUCCESS)
	{
		sem_post(&(handler->semaphore));
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
	handler->flag_exit_thread = true;
	//send a signal to make sure thread is not stuck at sem_wait
	sem_post(&(handler->semaphore));
	if (pthread_join(handler->thread_handler, NULL) != 0)
	{
		SLOGE(LOG_TAG, "error on join handle msg thread");
	}
	sem_destroy(&(handler->semaphore));
	msg_queue_destroy(&handler->msg_queue_p);
	handler->msg_queue_p = NULL;
	free(handler);
	*handler_p = NULL;
}
