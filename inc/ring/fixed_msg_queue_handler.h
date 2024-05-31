#pragma once
#ifndef LCU_FIXED_MSG_QUEUE_HANDLER_H
#define LCU_FIXED_MSG_QUEUE_HANDLER_H

#include <stdbool.h>
#include <stdint.h>
#include "ring/msg_queue_errno.h"

#ifndef __in
#define __in
#endif
#ifndef __out
#define __out
#endif
#ifndef __inout
#define __inout
#endif
#ifndef __in_opt
#define __in_opt
#endif
#ifndef __out_opt
#define __out_opt
#endif
#ifndef __inout_opt
#define __inout_opt
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MSG_OBJ_MAX_CAPACITY (1024)

	typedef char fixed_msg_obj_t;

	/**
	 * msg prototype
	 */
	typedef struct
	{
		int what;
		int arg1;
		int arg2;
		struct
		{
			int data_len;
			fixed_msg_obj_t data[MSG_OBJ_MAX_CAPACITY];
		} obj;
	} fixed_msg_t;

	typedef struct _fixed_msg_queue_handler_s* fixed_msg_queue_handler;

	typedef struct
	{
		/* user_data for callback */
		void* user_data;
		/**
		  * @brief callback prototype of handle msg.
		  *
		  * note: you shouldn't do too much time-consuming operation on here.
		  *
		  * @param[in] queue_msg_t  pointer to popped msg. do not freed this msg memory.
		  * @param[in] user_data    user data pointer that you passed in init_param
		  *
		  * @return 0 for normal status, otherwise will break the handler queue
		  */
		int (*process_msg)(fixed_msg_t* msg_p, void* user_data);
		void (*notify_msg_handler_status)(msg_q_handler_status_e status, void* user_data);
	} fixed_msg_queue_handler_init_param_t;

	/**
	 * @brief create fixed_queue_handler
	 *
	 * @param[in] max_msg_capacity   max capacity
	 * @param[in] param_p            pointer of fixed_msg_queue_handler_init_param_t.
	 * 
	 * @return queue handler ptr
	 */
	fixed_msg_queue_handler fixed_msg_queue_handler_create(__in uint32_t max_msg_capacity,
		__in fixed_msg_queue_handler_init_param_t *init_param_p);

	/**
	 * @brief push msg at tail of queue handler,
	 * you should lock this if you call it on multi-thread
	 *
	 * @param[in] handler queue handler ptr
	 * @param[in] msg_p   msg ptr
	 * 
	 * @return 0 succeed. otherwise failed
	 */
	msg_q_code_e fixed_msg_queue_handler_push(__in fixed_msg_queue_handler handler, __in fixed_msg_t* msg_p);

	/**
	 * @brief current available push msg amount
	 *
	 * @param[in] handler queue handler ptr
	 * 
	 * @return amount
	 */
	uint32_t fixed_msg_queue_handler_available_push_amount(__in fixed_msg_queue_handler handler);

	/**
	 * @brief current msg amount in queue handler
	 *
	 * @param[in] handler queue handler ptr
	 * 
	 * @return amount
	 */
	uint32_t fixed_msg_queue_handler_available_pop_amount(__in fixed_msg_queue_handler handler);

	/**
	 * @brief if current msg amount in queue handler is zero
	 *
	 * @param handler queue handler ptr
	 * @return true indicate for empty.
	 */
	bool fixed_msg_queue_handler_is_empty(__in fixed_msg_queue_handler handler);

	/**
	 * @brief if current can send msg max amount is zero.
	 *
	 * @param handler queue handler ptr
	 * @return true indicate queue is full
	 */
	bool fixed_msg_queue_handler_is_full(__in fixed_msg_queue_handler handler);

	/**
	 * @brief clear all msg in queue if msg has not handled.
	 * 
	 * note: usually it will be fast, but if you stuck in callback, 
	 *       it will effect after the latest callback finished.
	 *
	 * @param[in] handler queue handler ptr
	 */
	void fixed_msg_queue_handler_clear(__in fixed_msg_queue_handler handler);

	/**
	 * @brief destroy queue handler
	 *
	 * @param[in] handler queue handler ptr
	 */
	void fixed_msg_queue_handler_destroy(__inout fixed_msg_queue_handler* handler_p);

#ifdef __cplusplus
}
#endif

#endif // !LCU_FIXED_MSG_QUEUE_HANDLER_H
