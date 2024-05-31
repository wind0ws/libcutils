#pragma once
#ifndef LCU_MSG_QUEUE_HANDLER_H
#define LCU_MSG_QUEUE_HANDLER_H

#include <stdint.h>   /* for uint32_t */
#include "ring/msg_queue_errno.h"

#ifdef _WIN32
#include <sal.h> /* for in/out param */
#endif // _WIN32

#ifndef __in
#define __in
#endif
#ifndef __out
#define __out
#endif
#ifndef __inout
#define __inout
#endif
#ifndef __success
#define __success(expr) 
#endif

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * msg prototype
	 */
	typedef struct
	{
		int what;	 /* msg type  */
		int arg1;	 /* user arg1 */
		int arg2;	 /* user arg2 */
		int obj_len; /* length of obj */
		char obj[0]; /* must be last field of struct */
	} queue_msg_t;

	typedef struct _msg_queue_handler_s* msg_queue_handler;

	/* init param for msg_queue_handler_create */
	typedef struct  
	{
		/* user_data that will pass in callback function */
		void* user_data;
	    
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
		int (*fn_handle_msg)(queue_msg_t* msg_p, void* user_data);
		
		/**
		 * @brief callback prototype of notify handler status changed
		 * 
		 * note: do NOT call any function of msg_queue_handler on this function, 
		 *       otherwise may cause stuck on thread of msg_queue_handler.
		 * 
		 * @param[in] status   	   status of current msg_queue_handler
		 * @param[in] user_data    user data pointer that you passed in init_param
		 */
		void (*fn_on_status_changed)(msg_q_handler_status_e status, void* user_data);
	} msg_queue_handler_init_param_t;

	/**
	 * @brief create msg_queue_handler.
	 *
	 * @param[in] queue_buf_size    total size of memory to hold msg
	 * @param[in] param_p           pointer of msg_queue_handler_init_param_t.
	 *
	 * @return queue handler ptr if success, otherwise return null
	 */
	msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size,
		__in msg_queue_handler_init_param_t *param_p);

	/**
	 * @brief push msg at tail of queue handler.
	 *
	 * note: if you send msg on multi-thread, you should lock this method.
	 *
	 * @param[in]  handler  queue handler ptr
	 * @param[in]  msg_p    msg ptr, will copy it's memory to queue
	 *
	 * @return status. 0 succeed. otherwise failed(see error code details on msg_queue_errno.h)
	 */
	msg_q_code_e msg_queue_handler_push(__in msg_queue_handler handler, __in queue_msg_t* msg_p);

	/**
	 * @brief available push byte size.
	 *
	 * note: this is byte size which NOT completely equal msg count.
	 *
	 * @param[in] handler   queue handler ptr
	 *
	 * @return byte size
	 */
	uint32_t msg_queue_handler_available_push_bytes(__in msg_queue_handler handler);

	/**
	 * @brief available pop byte size.
	 *
	 * note: this is byte size which NOT completely equal msg count.
	 *
	 * @param[in] handler   queue handler ptr
	 *
	 * @return byte size
	 */
	uint32_t msg_queue_handler_available_pop_bytes(__in msg_queue_handler handler);

	/**
	 * @brief destroy queue handler.
	 *
	 * @param[in,out] handler   pointer of queue handler pointer
	 */
	void msg_queue_handler_destroy(__inout msg_queue_handler* handler_p);

#ifdef __cplusplus
}
#endif

#endif // !LCU_MSG_QUEUE_HANDLER_H
