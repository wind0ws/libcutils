#pragma once
#ifndef __LCU_RING_MSG_QUEUE_H
#define __LCU_RING_MSG_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ringbuffer.h"

typedef struct __ring_msg_queue *ring_msg_queue;

/**
 * create RingMsgQueue
 * @param max_msg_capacity max amount msg
 * @return RingMsgQueue pointer
 */
ring_msg_queue RingMsgQueue_create(__in uint32_t one_msg_byte_size, __in uint32_t max_msg_capacity);

/**
 * push QueueMsg to queue tail.
 * <p>if queue is full, push will be fail</p>
 * @param ring_msg_queue_p RingMsgQueue
 * @param msg_p the msg pointer which to read and copy it memory to queue tail
 * @return true indicator push succeed, otherwise false
 */
bool RingMsgQueue_push(__in ring_msg_queue ring_msg_queue_p, __in const void *msg_p);

/**
 * pop queue msg from head
 * @param ring_msg_queue_p RingMsgQueue
 * @param msg_p the msg pointer which will be write(copy memory from ring_msg_queue, so caller should manage msg memory first)
 * @return true indicator pop succeed, otherwise false
 */
bool RingMsgQueue_pop(__in ring_msg_queue ring_msg_queue_p, __in void *msg_p);

/**
 * clear all QueueMsg
 * <p>Warn: you should stop call push/pop method first before call this method, otherwise it will have thread safe issue.<br>
 * after call this method, you are free to call push/pop even at same time in two thread just like before.</p>
 * @param ring_msg_queue_p RingMsgQueue
 */
void RingMsgQueue_clear(__in ring_msg_queue ring_msg_queue_p);

/**
 * get current msg amount in msg queue
 * @param ring_msg_queue_p RingMsgQueue
 * @return msg amount
 */
uint32_t RingMsgQueue_available_pop_msg_amount(__in ring_msg_queue ring_msg_queue_p);

/**
 * get current max pushable msg amount
 * @param ring_msg_queue_p RingMsgQueue
 * @return msg amount
 */
uint32_t RingMsgQueue_available_push_msg_amount(__in ring_msg_queue ring_msg_queue_p);

/**
 * if msg amount in queue is zero
 * @param ring_msg_queue_p RingMsgQueue
 * @return true indicator queue is empty, otherwise false
 */
bool RingMsgQueue_is_empty(__in ring_msg_queue ring_msg_queue_p);

/**
 * if available push msg amount is zero
 * @param ring_msg_queue_p RingMsgQueue
 * @return true indicator queue is full, otherwise false
 */
bool RingMsgQueue_is_full(__in ring_msg_queue ring_msg_queue_p);

/**
 * destroy RingMsgQueue and free memory
 * <p>Warn: you should stop call push/pop first before call this method</p>
 * @param ring_msg_queue_p RingMsgQueue
 */
void RingMsgQueue_destroy(__in ring_msg_queue *ring_msg_queue_pp);

#ifdef __cplusplus
}
#endif

#endif //__LCU_RING_MSG_QUEUE_H
