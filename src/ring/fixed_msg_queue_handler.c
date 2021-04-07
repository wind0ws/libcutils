#include <malloc.h>
#include "thread/thread_wrapper.h"
#include "ring/fixed_msg_queue_handler.h"

typedef struct
{
	fixed_msg_t out_msg;
	size_t token;
} fixed_handler_msg_t;

struct __fixed_msg_queue_handler
{
	fixed_msg_queue msg_queue_p;
	fixed_msg_handler_callback callback;
	void* callback_userdata;
	pthread_t thread_handler;
	sem_t sem_handler;
	volatile bool flag_exit_thread;

	fixed_handler_msg_t msg_send_cache;
	size_t token_counter;
	volatile size_t min_valid_token;
};

static void* thread_fun_handle_msg(void* thread_context)
{
	fixed_msg_queue_handler handler_p = (fixed_msg_queue_handler)thread_context;
	fixed_handler_msg_t handler_msg = { {0},0 };
	for (;;)
	{
		sem_wait(&handler_p->sem_handler);
		if (handler_p->flag_exit_thread)
		{
			break;
		}
		if (!fixed_msg_queue_pop(handler_p->msg_queue_p, &handler_msg))
		{
			continue;
		}
		if (handler_msg.token >= handler_p->min_valid_token)
		{
			handler_p->callback(&handler_msg.out_msg, handler_p->callback_userdata);
		}
		else
		{
			RING_LOGW("abandon msg, token=%zd", handler_msg.token);
		}
	}
	return NULL;
}

fixed_msg_queue_handler fixed_msg_queue_handler_create(__in uint32_t max_msg_capacity,
	__in fixed_msg_handler_callback callback, __in void* callback_userdata)
{
	RING_LOGD("create fixed_queue_handler. max_msg_capacity=%d", max_msg_capacity);
	fixed_msg_queue_handler handler_p = calloc(1, sizeof(struct __fixed_msg_queue_handler));
	if (!handler_p)
	{
		return NULL;
	}
	handler_p->flag_exit_thread = false;
	handler_p->callback = callback;
	handler_p->callback_userdata = callback_userdata;
	handler_p->token_counter = 0;
	handler_p->min_valid_token = 0;
	sem_init(&(handler_p->sem_handler), 0, 0);
	if (pthread_create(&(handler_p->thread_handler), NULL, thread_fun_handle_msg, handler_p) == 0)
	{
		char thr_name[32] = { 0 };
		snprintf(thr_name, sizeof(thr_name), "q_hdl_%p", handler_p);
		pthread_setname_np(handler_p->thread_handler, thr_name);
		handler_p->msg_queue_p = fixed_msg_queue_create(sizeof(fixed_handler_msg_t), max_msg_capacity);
	}
	else
	{
		RING_LOGE("error on create pthread of queue handle msg");
		sem_destroy(&(handler_p->sem_handler));
		free(handler_p);
		handler_p = NULL;
	}
	return handler_p;
}

int fixed_msg_queue_handler_send(__in fixed_msg_queue_handler fixed_msg_queue_handler_p, __in fixed_msg_t* msg_p)
{
	if (!fixed_msg_queue_handler_p || fixed_msg_queue_handler_p->msg_queue_p == NULL || fixed_msg_queue_handler_p->flag_exit_thread)
	{
		return 1;
	}
	fixed_handler_msg_t* handler_msg_p = &(fixed_msg_queue_handler_p->msg_send_cache);
	handler_msg_p->out_msg = *msg_p;
	handler_msg_p->token = fixed_msg_queue_handler_p->token_counter;

	if (!fixed_msg_queue_push(fixed_msg_queue_handler_p->msg_queue_p, handler_msg_p))
	{
		//RING_LOGE("send msg to queue handled failed. queue is full");
		return 2;
	}
	fixed_msg_queue_handler_p->token_counter++;
	sem_post(&(fixed_msg_queue_handler_p->sem_handler));
	return 0;
}

extern inline uint32_t fixed_msg_queue_handler_available_send_msg_amount(__in fixed_msg_queue_handler handler_p)
{
	return fixed_msg_queue_available_push_msg_amount(handler_p->msg_queue_p);
}

extern inline uint32_t fixed_msg_queue_handler_current_queue_msg_amount(__in fixed_msg_queue_handler handler_p)
{
	return fixed_msg_queue_available_pop_msg_amount(handler_p->msg_queue_p);
}

extern inline bool fixed_msg_queue_handler_is_empty(__in fixed_msg_queue_handler handler_p)
{
	return fixed_msg_queue_handler_current_queue_msg_amount(handler_p) == 0;
}

extern inline bool fixed_msg_queue_handler_is_full(__in fixed_msg_queue_handler handler_p)
{
	return fixed_msg_queue_handler_available_send_msg_amount(handler_p) == 0;
}

extern inline void fixed_msg_queue_handler_clear(__in fixed_msg_queue_handler handler_p)
{
	handler_p->min_valid_token = handler_p->token_counter;
	RING_LOGD("fixed_msg_queue_handler_clear handler_p(%p) min_valid_token=%zd",
		handler_p, handler_p->min_valid_token);
}

void fixed_msg_queue_handler_destroy(__inout fixed_msg_queue_handler* handler_pp)
{
	if (!handler_pp || !(*handler_pp))
	{
		return;
	}
	fixed_msg_queue_handler handler_p = *handler_pp;
	handler_p->flag_exit_thread = true;
	//send a signal to make sure thread is not stuck at sem_wait
	sem_post(&(handler_p->sem_handler));
	if (pthread_join(handler_p->thread_handler, NULL) != 0)
	{
		RING_LOGE("error on join handle msg thread.");
	}
	sem_destroy(&(handler_p->sem_handler));
	fixed_msg_queue_destroy(&handler_p->msg_queue_p);
	handler_p->msg_queue_p = NULL;
	free(handler_p);
	*handler_pp = NULL;
}
