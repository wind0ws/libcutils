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
		int what;
		int arg1;
		int arg2;
		int obj_len;
		char obj[0]; /* must be last field of struct */
	} queue_msg_t;

	typedef struct _msg_queue_handler_s* msg_queue_handler;

	/**
	 * @brief callback prototype that handle msg.
	 * 
	 * note: you shouldn't do too much time-consuming operation on here.
	 *
	 * @param[in] queue_msg_t  pointer to popped msg. do not freed this msg memory.
	 * @param[in] user_data    user data pointer that you passed in msg_queue_handler_create()
	 *
	 * @return 0 for normal status, otherwise will break the handler queue
	 */
	typedef int (*msg_handler_callback_t)(queue_msg_t* msg_p, void* user_data);

	/**
	 * @brief create msg_queue_handler.
	 *
	 * @param[in] queue_buf_size    total size of memory to hold msg
	 * @param[in] callback          handle msg function
	 * @param[in] callback_userdata user_data will pass in callback
	 *
	 * @return queue handler ptr if success, otherwise return null
	 */
	msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size,
		__in msg_handler_callback_t callback, __in void* callback_userdata);

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
	MSG_Q_CODE msg_queue_handler_push(__in msg_queue_handler handler, __in queue_msg_t* msg_p);

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
