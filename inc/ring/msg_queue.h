#pragma once
#ifndef __LCU_MSG_QUEUE_H
#define __LCU_MSG_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ringbuffer.h"

typedef struct __msg_queue *msg_queue;

/**
 * create msg_queue
 * @param buf_size total buffer size
 * @return msg_queue pointer
 */
msg_queue msg_queue_create(__in uint32_t buf_size);

/**
 * push QueueMsg to queue tail.
 * <p>if queue is full, push will be fail</p>
 * @param msg_queue_p msg_queue
 * @param msg_p the msg pointer which to read and copy it memory to queue tail
 * @return true indicator push succeed, otherwise false
 */
bool msg_queue_push(__in msg_queue msg_queue_p, __in const void *msg_p, __in const uint32_t msg_size);


/**
 * get next msg size from queue.
 * you can use this size to create msg memory
 * @return next queue msg real size
 */
uint32_t msg_queue_next_msg_size(__in msg_queue msg_queue_p);

/**
 * pop queue msg from head
 * @param msg_queue_p msg_queue
 * @param msg_p the msg pointer which will be write(copy memory from msg_queue, so caller should manage msg memory first)
 * @param msg_size_p allocated memory size pointer of msg_p. this size will changed by this function, caller can use this to know real memory size of msg_p
 * @return true indicator pop succeed, otherwise false
 */
bool msg_queue_pop(__in msg_queue msg_queue_p, __inout void *msg_p, __inout uint32_t *msg_size_p);

/**
 * clear all QueueMsg
 * <p>Warn: you should stop call push/pop method first before call this method, otherwise it will have thread safe issue.<br>
 * after call this method, you are free to call push/pop even at same time in two thread just like before.</p>
 * @param msg_queue_p msg_queue
 */
void msg_queue_clear(__in msg_queue msg_queue_p);

/**
 * get current msg amount in msg queue
 * @param msg_queue_p msg_queue
 * @return msg amount
 */
uint32_t msg_queue_available_pop_bytes(__in msg_queue msg_queue_p);

/**
 * get current max pushable msg amount
 * @param msg_queue_p msg_queue
 * @return msg amount
 */
uint32_t msg_queue_available_push_bytes(__in msg_queue msg_queue_p);

/**
 * if msg amount in queue is zero
 * @param msg_queue_p msg_queue
 * @return true indicator queue is empty, otherwise false
 */
bool msg_queue_is_empty(__in msg_queue msg_queue_p);

/**
 * if available push msg amount is zero
 * @param msg_queue_p msg_queue
 * @return true indicator queue is full, otherwise false
 */
bool msg_queue_is_full(__in msg_queue msg_queue_p);

/**
 * destroy msg_queue and free memory
 * <p>Warn: you should stop call push/pop first before call this method</p>
 * @param msg_queue_p msg_queue
 */
void msg_queue_destroy(__in msg_queue *msg_queue_pp);

#ifdef __cplusplus
}
#endif

#endif //__LCU_MSG_QUEUE_H
