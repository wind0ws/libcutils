#include "common_macro.h"
#include "ring/msg_queue_handler.h"
#include "ring/msg_queue.h"
#include "log/simple_log.h"
#include <malloc.h>
#include "thread/thread_wrapper.h"

#define LOG_TAG "MSG_Q_HDL"

struct __msg_queue_handler
{
	msg_queue msg_queue_p;
	msg_handler_callback callback;
	void* callback_userdata;
	pthread_t thread_handler;
	sem_t sem_handler;
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
	msg_queue_handler handler_p = (msg_queue_handler)thread_context;
	size_t cur_msg_buf_size = 2048;
	char* poped_msg_buf = (char *)malloc(cur_msg_buf_size);
	if (!poped_msg_buf)
	{
		SIMPLE_LOGE(LOG_TAG, "can't malloc %zu on thread_fun_handle_msg, now thread exit...", cur_msg_buf_size);
		return NULL;
	}
	MSG_Q_CODE last_status = MSG_Q_CODE_SUCCESS;
	for (;;)
	{
		if (last_status == MSG_Q_CODE_SUCCESS || last_status == MSG_Q_CODE_EMPTY)
		{
			sem_wait(&handler_p->sem_handler);
		}
		if (handler_p->flag_exit_thread)
		{
			break;
		}
		uint32_t msg_size = cur_msg_buf_size;
		last_status = msg_queue_pop(handler_p->msg_queue_p, poped_msg_buf, &msg_size);
		if (last_status != MSG_Q_CODE_SUCCESS)
		{
			if (last_status == MSG_Q_CODE_BUF_NOT_ENOUGH)
			{
				free(poped_msg_buf);
				size_t next_msg_size = msg_queue_next_msg_size(handler_p->msg_queue_p);
				ASSERT_ABORT(next_msg_size > 0);
				size_t expect_buf_size = roundup_power2(next_msg_size);
				poped_msg_buf = (char*)malloc(expect_buf_size);
				if (!poped_msg_buf)
				{
					SIMPLE_LOGE(LOG_TAG, "can't malloc %zu on thread_fun_handle_msg", expect_buf_size);
					break;
				}
				cur_msg_buf_size = expect_buf_size;
			}
			continue;
		}
		queue_msg_t* msg = (queue_msg_t*)poped_msg_buf;
		handler_p->callback(msg, handler_p->callback_userdata);
	}
	if (poped_msg_buf)
	{
		free(poped_msg_buf);
	}
	return NULL;
}

msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size,
	__in msg_handler_callback callback, __in void* callback_userdata)
{
	SIMPLE_LOGD(LOG_TAG, "create msg_queue_handler. queue_buf_size=%d", queue_buf_size);
	msg_queue_handler handler_p = calloc(1, sizeof(struct __msg_queue_handler));
	if (!handler_p)
	{
		return NULL;
	}
	handler_p->flag_exit_thread = false;
	handler_p->callback = callback;
	handler_p->callback_userdata = callback_userdata;
	sem_init(&(handler_p->sem_handler), 0, 0);
	if (pthread_create(&(handler_p->thread_handler), NULL, thread_fun_handle_msg, handler_p) == 0)
	{
		char thr_name[32] = { 0 };
		snprintf(thr_name, sizeof(thr_name), "q_hdl_%p", handler_p);
		pthread_setname_np(handler_p->thread_handler, thr_name);
		handler_p->msg_queue_p = msg_queue_create(queue_buf_size);
	}
	else
	{
		SIMPLE_LOGE(LOG_TAG, "error on create pthread of queue handle msg");
		sem_destroy(&(handler_p->sem_handler));
		free(handler_p);
		handler_p = NULL;
	}
	return handler_p;
}

MSG_Q_CODE msg_queue_handler_send(__in msg_queue_handler handler_p, __in queue_msg_t* msg_p)
{
	if (!handler_p || handler_p->msg_queue_p == NULL || handler_p->flag_exit_thread)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	MSG_Q_CODE push_status = msg_queue_push(handler_p->msg_queue_p, msg_p, (sizeof(queue_msg_t) + msg_p->obj_len));
	if (push_status == MSG_Q_CODE_SUCCESS)
	{
		sem_post(&(handler_p->sem_handler));
	}
	return push_status;
}

extern inline uint32_t msg_queue_handler_available_push_bytes(__in msg_queue_handler handler_p)
{
	return msg_queue_available_push_bytes(handler_p->msg_queue_p);
}

extern inline uint32_t msg_queue_handler_available_pop_bytes(__in msg_queue_handler handler_p)
{
	return msg_queue_available_pop_bytes(handler_p->msg_queue_p);
}

void msg_queue_handler_destroy(__inout msg_queue_handler* handler_pp)
{
	if (!handler_pp || !(*handler_pp))
	{
		return;
	}
	msg_queue_handler handler_p = *handler_pp;
	handler_p->flag_exit_thread = true;
	//send a signal to make sure thread is not stuck at sem_wait
	sem_post(&(handler_p->sem_handler));
	if (pthread_join(handler_p->thread_handler, NULL) != 0)
	{
		SIMPLE_LOGE(LOG_TAG, "error on join handle msg thread.");
	}
	sem_destroy(&(handler_p->sem_handler));
	msg_queue_destroy(&handler_p->msg_queue_p);
	handler_p->msg_queue_p = NULL;
	free(handler_p);
	*handler_pp = NULL;
}
