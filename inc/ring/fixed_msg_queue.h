#pragma once
#ifndef LCU_FIXED_MSG_QUEUE_H
#define LCU_FIXED_MSG_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

#ifndef __in
#define __in
#endif
#ifndef __out
#define __out
#endif
#ifndef __inout
#define __inout
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __fixed_msg_queue *fixed_msg_queue;

/**
 * create fixed_msg_queue
 * @param max_msg_capacity max amount msg
 * @return fixed_msg_queue pointer
 */
fixed_msg_queue fixed_msg_queue_create(__in uint32_t one_msg_byte_size, __in uint32_t max_msg_capacity);

/**
 * push QueueMsg to queue tail.
 * <p>if queue is full, push will be fail</p>
 * @param fixed_msg_queue_p fixed_msg_queue
 * @param msg_p the msg pointer which to read and copy it memory to queue tail
 * @return true indicator push succeed, otherwise false
 */
bool fixed_msg_queue_push(__in fixed_msg_queue fixed_msg_queue_p, __in const void *msg_p);

/**
 * pop queue msg from head
 * @param fixed_msg_queue_p fixed_msg_queue
 * @param msg_p the msg pointer which will be write(copy memory from fixed_msg_queue, so caller should manage msg memory first)
 * @return true indicator pop succeed, otherwise false
 */
bool fixed_msg_queue_pop(__in fixed_msg_queue fixed_msg_queue_p, __in void *msg_p);

/**
 * clear all QueueMsg
 * <p>Warn: you should stop call push/pop method first before call this method, otherwise it will have thread safe issue.<br>
 * after call this method, you are free to call push/pop even at same time in two thread just like before.</p>
 * @param fixed_msg_queue_p fixed_msg_queue
 */
void fixed_msg_queue_clear(__in fixed_msg_queue fixed_msg_queue_p);

/**
 * get current msg amount in msg queue
 * @param fixed_msg_queue_p fixed_msg_queue
 * @return msg amount
 */
uint32_t fixed_msg_queue_available_pop_msg_amount(__in fixed_msg_queue fixed_msg_queue_p);

/**
 * get current max pushable msg amount
 * @param fixed_msg_queue_p fixed_msg_queue
 * @return msg amount
 */
uint32_t fixed_msg_queue_available_push_msg_amount(__in fixed_msg_queue fixed_msg_queue_p);

/**
 * if msg amount in queue is zero
 * @param fixed_msg_queue_p fixed_msg_queue
 * @return true indicator queue is empty, otherwise false
 */
bool fixed_msg_queue_is_empty(__in fixed_msg_queue fixed_msg_queue_p);

/**
 * if available push msg amount is zero
 * @param fixed_msg_queue_p fixed_msg_queue
 * @return true indicator queue is full, otherwise false
 */
bool fixed_msg_queue_is_full(__in fixed_msg_queue fixed_msg_queue_p);

/**
 * destroy fixed_msg_queue and free memory
 * <p>Warn: you should stop call push/pop first before call this method</p>
 * @param fixed_msg_queue_p fixed_msg_queue
 */
void fixed_msg_queue_destroy(__inout fixed_msg_queue *fixed_msg_queue_pp);

#ifdef __cplusplus
}
#endif

#endif // LCU_FIXED_MSG_QUEUE_H
