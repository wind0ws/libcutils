#include <malloc.h>
#include "thread/thread_wrapper.h"
#include "ring/msg_queue_handler.h"

typedef struct
{
	queue_msg_t out_msg;
	size_t token;
} handler_msg_t;

struct __queue_handler
{
	ring_msg_queue msg_queue_p;
	msg_handler_callback callback;
	void* callback_userdata;
	pthread_t thread_handler;
	sem_t sem_handler;
	bool flag_exit_thread;

	handler_msg_t msg_send_cache;
	size_t token_counter;
	size_t min_valid_token;
};

static void* thread_fun_handle_msg(void* thread_context)
{
	queue_handler handler_p = (queue_handler)thread_context;
	handler_msg_t handler_msg = { 0 };
	while (!(handler_p->flag_exit_thread))
	{
		sem_wait(&handler_p->sem_handler);
		if (handler_p->flag_exit_thread)
		{
			break;
		}
		if (!RingMsgQueue_pop(handler_p->msg_queue_p, &handler_msg))
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

queue_handler QueueHandler_create(__in uint32_t max_msg_capacity,
	__in msg_handler_callback callback, __in void* callback_userdata)
{
	RING_LOGD("create queue_handler. max_msg_capacity=%d", max_msg_capacity);
	queue_handler handler_p = calloc(1, sizeof(struct __queue_handler));
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
		handler_p->msg_queue_p = RingMsgQueue_create(sizeof(handler_msg_t), max_msg_capacity);
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

int QueueHandler_send(__in queue_handler queue_handler_p, __in queue_msg_t* msg_p)
{
	if (!queue_handler_p || queue_handler_p->msg_queue_p == NULL || queue_handler_p->flag_exit_thread)
	{
		return 1;
	}
	handler_msg_t* handler_msg_p = &(queue_handler_p->msg_send_cache);
	handler_msg_p->out_msg = *msg_p;
	handler_msg_p->token = queue_handler_p->token_counter;

	if (!RingMsgQueue_push(queue_handler_p->msg_queue_p, handler_msg_p))
	{
		//RING_LOGE("send msg to queue handled failed. queue is full");
		return 2;
	}
	queue_handler_p->token_counter++;
	sem_post(&(queue_handler_p->sem_handler));
	return 0;
}

extern inline uint32_t QueueHandler_available_send_msg_amount(__in queue_handler handler_p)
{
	return RingMsgQueue_available_push_msg_amount(handler_p->msg_queue_p);
}

extern inline uint32_t QueueHandler_current_queue_msg_amount(__in queue_handler handler_p)
{
	return RingMsgQueue_available_pop_msg_amount(handler_p->msg_queue_p);
}

extern inline bool QueueHandler_is_empty(__in queue_handler handler_p)
{
	return QueueHandler_current_queue_msg_amount(handler_p) == 0;
}

extern inline bool QueueHandler_is_full(__in queue_handler handler_p)
{
	return QueueHandler_available_send_msg_amount(handler_p) == 0;
}

extern inline void QueueHandler_clear(__in queue_handler handler_p)
{
	handler_p->min_valid_token = handler_p->token_counter;
	RING_LOGD("QueueHandler_clear handler_p(%p) min_valid_token=%zd", handler_p,
		handler_p->min_valid_token);
}

void QueueHandler_destroy(__inout queue_handler *handler_pp)
{
	if (!handler_pp || !(*handler_pp))
	{
		return;
	}
	queue_handler handler_p = *handler_pp;
	handler_p->flag_exit_thread = true;
	//send a signal to make sure thread is not stuck at sem_wait
	sem_post(&(handler_p->sem_handler));
	if (pthread_join(handler_p->thread_handler, NULL) != 0)
	{
		RING_LOGE("error on join handle msg thread.");
	}
	sem_destroy(&(handler_p->sem_handler));
	RingMsgQueue_destroy(&handler_p->msg_queue_p);
	handler_p->msg_queue_p = NULL;
	free(handler_p);
	*handler_pp = NULL;
}
