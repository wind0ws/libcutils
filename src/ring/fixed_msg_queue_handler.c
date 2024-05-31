#include "mem/mem_debug.h"
#include "log/slog.h"
#include "ring/fixed_msg_queue.h"
#include "ring/fixed_msg_queue_handler.h"
#include "thread/portable_thread.h"
#include <string.h>
#include <malloc.h>

#define Q_LOG_TAG         "FIXED_Q_HDL"

#define MY_LOGV(fmt,...)  SLOGV(Q_LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGD(fmt,...)  SLOGD(Q_LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGI(fmt,...)  SLOGI(Q_LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGW(fmt,...)  SLOGW(Q_LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGE(fmt,...)  SLOGE(Q_LOG_TAG, fmt, ##__VA_ARGS__)

typedef struct
{
	size_t token;
	fixed_msg_t out_msg;
} fixed_handler_msg_t;

struct _fixed_msg_queue_handler_s
{
	fixed_msg_queue_handler_init_param_t param;
	fixed_msg_queue msg_queue_p;

	volatile bool flag2exit;
	portable_thread_t thread_handler;
	portable_sem_t semaphore;

	fixed_handler_msg_t msg_send_cache;
	size_t token_counter;
	volatile size_t min_valid_token;
};

static void pri_notify_status(fixed_msg_queue_handler handler, msg_q_handler_status_e status)
{
	if (NULL == handler->param.notify_msg_handler_status)
	{
		return;
	}
	handler->param.notify_msg_handler_status(status, handler->param.user_data);
}

static void* thread_fun_handle_msg(void* thread_context)
{
	fixed_msg_queue_handler handler = (fixed_msg_queue_handler)thread_context;
	MY_LOGI(" %s:%d thread(%lu) started...", __func__, __LINE__, GETTID());
	pri_notify_status(handler, MSG_Q_HANDLER_STATUS_STARTED);

	fixed_handler_msg_t handler_msg = { 0, {0} };
	int user_handle_ret;
	for (;;)
	{
		portable_sem_wait(&handler->semaphore);
		if (handler->flag2exit)
		{
			break;
		}
		if (!fixed_msg_queue_pop(handler->msg_queue_p, &handler_msg))
		{
			continue;
		}
		if (handler_msg.token >= handler->min_valid_token)
		{
			if (0 != (user_handle_ret = handler->param.process_msg(&handler_msg.out_msg, handler->param.user_data)))
			{
				MY_LOGE("error(%d) on user process msg, now exit", user_handle_ret);
				break;
			}
		}
		else
		{
			MY_LOGW("abandon msg: token=%zu, min_valid_token=%zu", 
				handler_msg.token, handler->min_valid_token);
		}
	}

	handler->flag2exit = true;// <-- for mark thread not handle msg yet!
	MY_LOGI(" %s:%d thread(%lu) exited...", __func__, __LINE__, GETTID());
	pri_notify_status(handler, MSG_Q_HANDLER_STATUS_STOPPED);
	return NULL;
}

fixed_msg_queue_handler fixed_msg_queue_handler_create(__in uint32_t max_msg_capacity,
	__in fixed_msg_queue_handler_init_param_t* init_param_p)
{
	if (max_msg_capacity < 2U || NULL == init_param_p || NULL == init_param_p->process_msg)
	{
		MY_LOGD("invalid param. max_msg_capacity=%u", max_msg_capacity);
		return NULL;
	}
	MY_LOGD("create fixed_queue_handler. max_msg_capacity=%u", max_msg_capacity);
	fixed_msg_queue_handler handler = (fixed_msg_queue_handler)calloc(1, sizeof(struct _fixed_msg_queue_handler_s));
	if (!handler)
	{
		return NULL;
	}
	handler->flag2exit = false;
	memcpy(&handler->param, init_param_p, sizeof(fixed_msg_queue_handler_init_param_t));
	handler->token_counter = 0;
	handler->min_valid_token = 0;
	portable_sem_init(&(handler->semaphore), 0, 0);
	handler->msg_queue_p = fixed_msg_queue_create(sizeof(fixed_handler_msg_t), max_msg_capacity);
	if (!handler->msg_queue_p || 
		0 != portable_thread_create(&(handler->thread_handler), NULL, thread_fun_handle_msg, handler))
	{
		MY_LOGE("error on create thread or queue!");
		fixed_msg_queue_handler_destroy(&handler);
		handler = NULL;
	}
	return handler;
}

msg_q_code_e fixed_msg_queue_handler_push(__in fixed_msg_queue_handler handler, __in fixed_msg_t* msg_p)
{
	if (!handler || !(handler->msg_queue_p) || handler->flag2exit)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	// um... here we should lock it on multi-thread, let user do it
	fixed_handler_msg_t* handler_msg_p = &(handler->msg_send_cache);
	handler_msg_p->out_msg = *msg_p; // perform copy inside
	handler_msg_p->token = handler->token_counter;

	if (!fixed_msg_queue_push(handler->msg_queue_p, handler_msg_p))
	{
		//MY_LOGE("send msg to queue handled failed. queue is full");
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
	return fixed_msg_queue_handler_available_pop_amount(handler) == 0;
}

extern inline bool fixed_msg_queue_handler_is_full(__in fixed_msg_queue_handler handler)
{
	return fixed_msg_queue_handler_available_push_amount(handler) == 0;
}

extern inline void fixed_msg_queue_handler_clear(__in fixed_msg_queue_handler handler)
{
	handler->min_valid_token = handler->token_counter;
	MY_LOGD("fixed_msg_queue_handler_clear handler(%p) min_valid_token=%zu",
		handler, handler->min_valid_token);
}

void fixed_msg_queue_handler_destroy(__inout fixed_msg_queue_handler* handler_p)
{
	if (!handler_p || !(*handler_p))
	{
		return;
	}
	fixed_msg_queue_handler handler = *handler_p;
	handler->flag2exit = true;
	//send a signal to make sure thread is not stuck at sem_wait()
	portable_sem_post(&(handler->semaphore));
	if (handler->thread_handler && 
		0 != portable_thread_join(handler->thread_handler, NULL))
	{
		MY_LOGE("error on join handle msg thread.");
	}
	portable_sem_destroy(&(handler->semaphore));
	if (handler->msg_queue_p)
	{
		fixed_msg_queue_destroy(&(handler->msg_queue_p));
	}
	handler->msg_queue_p = NULL;
	free(handler);
	*handler_p = NULL;
}
