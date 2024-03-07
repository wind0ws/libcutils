#pragma once
#ifndef LCU_FIXED_MSG_QUEUE_H
#define LCU_FIXED_MSG_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

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

	typedef struct _fixed_msg_queue_s* fixed_msg_queue;

	/**
	 * @brief create fixed_msg_queue
	 *
	 * @param[in] max_msg_capacity max amount msg
	 *
	 * @return fixed_msg_queue pointer
	 */
	fixed_msg_queue fixed_msg_queue_create(__in uint32_t one_msg_byte_size, __in uint32_t max_msg_capacity);

	/**
	 * @brief push queue msg to tail.
	 *
	 * if queue is full, push will be fail.
	 *
	 * @param[in] msg_queue fixed_msg_queue
	 * @param[in] msg_p the msg pointer which to read and copy it memory to queue tail
	 *
	 * @return true indicator push succeed, otherwise false
	 */
	bool fixed_msg_queue_push(__in fixed_msg_queue msg_queue, __in const void* msg_p);

	/**
	 * @brief pop queue msg from head.
	 *
	 * if queue is empty, pop will be fail.
	 *
	 * @param[in]  msg_queue fixed_msg_queue
	 * @param[out] msg_p the msg pointer which will be write
	 *            (copy memory from fixed_msg_queue, so caller should manage msg memory first)
	 *
	 * @return true indicator pop succeed, otherwise false
	 */
	__success(return == true)
	bool fixed_msg_queue_pop(__in fixed_msg_queue msg_queue, __out void* msg_p);

	/**
	 * @brief clear all queue msg
	 *
	 * warn: you should stop call push/pop method first before call this method,
	 * otherwise it will have thread safe issue.
	 * after call this method, you are free to call push/pop even
	 * at same time in two thread just like before.
	 *
	 * @param[in] msg_queue fixed_msg_queue
	 */
	void fixed_msg_queue_clear(__in fixed_msg_queue msg_queue);

	/**
	 * @brief get current msg amount in msg queue
	 *
	 * @param[in] msg_queue fixed_msg_queue
	 *
	 * @return msg amount
	 */
	uint32_t fixed_msg_queue_available_pop_amount(__in fixed_msg_queue msg_queue);

	/**
	 * @brief get current max pushable msg amount
	 *
	 * @param[in] msg_queue fixed_msg_queue
	 *
	 * @return msg amount
	 */
	uint32_t fixed_msg_queue_available_push_amount(__in fixed_msg_queue msg_queue);

	/**
	 * @brief if msg amount in queue is zero
	 *
	 * @param[in] msg_queue fixed_msg_queue
	 *
	 * @return true indicator queue is empty, otherwise false
	 */
	bool fixed_msg_queue_is_empty(__in fixed_msg_queue msg_queue);

	/**
	 * @brief if available push msg amount is zero
	 *
	 * @param[in] msg_queue fixed_msg_queue
	 *
	 * @return true indicator queue is full, otherwise false
	 */
	bool fixed_msg_queue_is_full(__in fixed_msg_queue msg_queue);

	/**
	 * @brief destroy fixed_msg_queue and free it's memory.
	 *
	 * warn: you should stop call push/pop first before call this method.
	 *
	 * @param[in,out] msg_queue fixed_msg_queue
	 */
	void fixed_msg_queue_destroy(__inout fixed_msg_queue* msg_queue_p);

#ifdef __cplusplus
}
#endif

#endif // !LCU_FIXED_MSG_QUEUE_H
