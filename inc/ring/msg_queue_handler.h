#pragma once
#ifndef __LCU_MSG_QUEUE_HANDLER_H
#define __LCU_MSG_QUEUE_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ring_msg_queue.h"

#define MSG_OBJ_MAX_CAPACITY (1024)
typedef char MSG_OBJ_DATA_TYPE;

/**
 * msg prototype
 */
typedef struct {
    int what;
    int arg1;
    int arg2;
    struct {
        MSG_OBJ_DATA_TYPE data[MSG_OBJ_MAX_CAPACITY];
        int data_len;
    } obj;
} queue_msg_t;

typedef struct __queue_handler *queue_handler;

/**
 * callback to handle msg
 */
typedef void (*msg_handler_callback)(queue_msg_t *msg_p, void *user_data);

/**
 * create QueueHandler
 * @param max_msg_capacity MaxMsgCapacity
 * @param callback handle msg function
 * @return queue handler ptr
 */
queue_handler QueueHandler_create(__in uint32_t max_msg_capacity, __in msg_handler_callback callback, __in void* callback_userdata);

/**
 * send msg to queue handler
 * @param handler_p queue handler ptr
 * @param msg_p msg ptr
 * @return 0 succeed. otherwise failed
 */
int QueueHandler_send(__in queue_handler handler_p,__in queue_msg_t *msg_p);

/**
 * current can send msg max amount
 * @param handler_p queue handler ptr
 * @return amount
 */
uint32_t QueueHandler_available_send_msg_amount(__in queue_handler handler_p);

/**
 * current msg amount in queue handler
 * @param handler_p queue handler ptr
 * @return amount
 */
uint32_t QueueHandler_current_queue_msg_amount(__in queue_handler handler_p);

/**
 * if current msg amount in queue handler is zero
 * @param handler_p queue handler ptr
 * @return true indicate for empty.
 */
bool QueueHandler_is_empty(__in queue_handler handler_p);

/**
 * if current can send msg max amount is zero.
 * @param handler_p queue handler ptr
 * @return true indicate queue is full
 */
bool QueueHandler_is_full(__in queue_handler handler_p);

/**
 * clear all msg in queue if msg has not handled.
 * <p>usually it will be fast, but if you stuck in callback, it will effect after the latest callback finished </p>
 * @param handler_p queue handler ptr
 */
void QueueHandler_clear(__in queue_handler handler_p);

/**
 * destroy queue handler
 * @param handler_p queue handler ptr
 */
void QueueHandler_destroy(__inout queue_handler *handler_pp);

#ifdef __cplusplus
}
#endif

#endif //__LCU_MSG_QUEUE_HANDLER_H
