#pragma once
#ifndef LCU_MSG_QUEUE_H
#define LCU_MSG_QUEUE_H

#include "ring/msg_queue_errno.h"
#include <stdint.h>               /* for uint32_t */

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

	typedef struct __msg_queue* msg_queue;

	/**
	 * @brief create msg_queue.
	 *
	 * @param[in] buf_size total buffer size
	 *
	 * @return msg_queue pointer
	 */
	msg_queue msg_queue_create(__in uint32_t buf_size);

	/**
	 * @brief push msg to queue tail.
	 * note: if queue is full, push will be fail.
	 *
	 * @param[in] msg_queue_p msg_queue
	 * @param[in] msg_p       the msg pointer which to read and copy it memory to queue tail
	 *
	 * @return see MSG_Q_CODE
	 */
	MSG_Q_CODE msg_queue_push(__in msg_queue msg_queue_p, __in const void* msg_p, __in const uint32_t msg_size);

	/**
	 * @brief get next msg size from queue(just peek it size, not pop it).
	 * you can use this size to create msg memory for pop
	 * 
	 * @param[in] msg_queue_p     msg_queue
	 * 
	 * @return next queue msg real size(0 means no next msg)
	 */
	uint32_t msg_queue_next_msg_size(__in msg_queue msg_queue_p);

	/**
	 * @brief pop msg from queue head
	 *
	 * @param[in]  msg_queue_p    msg_queue
	 * @param[out] msg_p          the msg pointer which will be write
	 *                            (copy memory from msg_queue, so caller should manage msg memory first)
	 * @param[in,out] msg_size_p  allocated memory size pointer of msg_p. this size will changed by this function,
						          caller can use this to know real memory size of popped msg

	 * @return see MSG_Q_CODE
	 */
	__success(return == MSG_Q_CODE_SUCCESS)
	MSG_Q_CODE msg_queue_pop(__in msg_queue msg_queue_p, __out void* msg_p, __inout uint32_t* msg_size_p);

	/**
	 * @brief clear all queue msg.
	 *
	 * warn: you should stop call push/pop method first before call this method,
	 * otherwise it will have thread safe issue.
	 * after call this method, you are free to call push/pop even at same time in two thread just like before.
	 *
	 * @param[in] msg_queue_p msg_queue
	 */
	void msg_queue_clear(__in msg_queue msg_queue_p);

	/**
	 * @brief get current queue used bytes.
	 *
	 * @param[in] msg_queue_p msg_queue
	 *
	 * @return byte size can be pop.
	 */
	uint32_t msg_queue_available_pop_bytes(__in msg_queue msg_queue_p);

	/**
	 * @brief get current queue unused bytes.
	 *
	 * @param[in] msg_queue_p msg_queue
	 *
	 * @return byte size can be push.
	 */
	uint32_t msg_queue_available_push_bytes(__in msg_queue msg_queue_p);

	/**
	 * @brief destroy msg_queue and free memory
	 * warn: you should stop call push/pop first before call this method.
	 *
	 * @param[in,out] msg_queue_p msg_queue pointer's pointer
	 */
	void msg_queue_destroy(__inout msg_queue* msg_queue_pp);

#ifdef __cplusplus
}
#endif

#endif // LCU_MSG_QUEUE_H
