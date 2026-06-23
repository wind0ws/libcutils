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

typedef enum
{
	FIXED_MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY = 0,	  /* 正常销毁: 退出阶段的队列消息将被丢弃 */
	FIXED_MSG_Q_HANDLER_DESTROY_FLAGS_GRACEFULLY = 1, /* 优雅销毁: 退出阶段的队列消息仍交给用户处理 */
} fixed_msg_q_handler_destroy_flags_e;

#ifdef __cplusplus
extern "C"
{
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

	typedef struct _fixed_msg_queue_handler_s *fixed_msg_queue_handler;

	typedef struct
	{
		struct
		{
			/* user_data that will pass in callback function */
			void *user_data;

			/**
			 * @brief callback prototype of handle msg.
			 *
			 * note: you shouldn't do too much time-consuming operation on here.
			 *
			 * @param[in] msg_p        pointer to popped msg. do not freed this msg memory.
			 * @param[in] user_data    user data pointer that you passed in init_param
			 *
			 * @return 0 for normal status, otherwise will break the handler queue
			 */
			int (*fn_handle_msg)(fixed_msg_t *msg_p, void *user_data);

			/**
			 * @brief callback prototype of notify handler status changed
			 *
			 * note: do NOT call any function of msg_queue_handler on this function,
			 *       otherwise may cause stuck on thread of msg_queue_handler.
			 *
			 * @param[in] status   	   status of current msg_queue_handler
			 * @param[in] user_data    user data pointer that you passed in init_param
			 */
			void (*fn_on_status_changed)(msg_q_handler_status_e status, void *user_data);
		} callback;
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
	msg_q_code_e fixed_msg_queue_handler_push(__in fixed_msg_queue_handler handler, __in fixed_msg_t *msg_p);

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
	 * @param[in] flags destroy flags, see fixed_msg_q_handler_destroy_flags_e
	 */
	void fixed_msg_queue_handler_destroy(__inout fixed_msg_queue_handler *handler_p, __in int flags);

#ifdef __cplusplus
}
#endif

#endif // !LCU_FIXED_MSG_QUEUE_HANDLER_H
