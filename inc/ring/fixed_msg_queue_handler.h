#pragma once
#ifndef LCU_FIXED_MSG_QUEUE_HANDLER_H
#define LCU_FIXED_MSG_QUEUE_HANDLER_H

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
typedef char MSG_OBJ_DATA_TYPE;

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
        MSG_OBJ_DATA_TYPE data[MSG_OBJ_MAX_CAPACITY];
        int data_len;
    } obj;
} fixed_msg_t;

typedef struct __fixed_msg_queue_handler *fixed_msg_queue_handler;

/**
 * callback to handle msg
 */
typedef void (*fixed_msg_handler_callback)(fixed_msg_t *msg_p, void *user_data);

/**
 * create fixed_queue_handler
 * @param max_msg_capacity MaxMsgCapacity
 * @param callback handle msg function
 * @return queue handler ptr
 */
fixed_msg_queue_handler fixed_msg_queue_handler_create(__in uint32_t max_msg_capacity, __in fixed_msg_handler_callback callback, __in void* callback_userdata);

/**
 * send msg to queue handler
 * @param handler_p queue handler ptr
 * @param msg_p msg ptr
 * @return 0 succeed. otherwise failed
 */
int fixed_msg_queue_handler_send(__in fixed_msg_queue_handler handler_p, __in fixed_msg_t *msg_p);

/**
 * current can send msg max amount
 * @param handler_p queue handler ptr
 * @return amount
 */
uint32_t fixed_msg_queue_handler_available_send_msg_amount(__in fixed_msg_queue_handler handler_p);

/**
 * current msg amount in queue handler
 * @param handler_p queue handler ptr
 * @return amount
 */
uint32_t fixed_msg_queue_handler_current_queue_msg_amount(__in fixed_msg_queue_handler handler_p);

/**
 * if current msg amount in queue handler is zero
 * @param handler_p queue handler ptr
 * @return true indicate for empty.
 */
bool fixed_msg_queue_handler_is_empty(__in fixed_msg_queue_handler handler_p);

/**
 * if current can send msg max amount is zero.
 * @param handler_p queue handler ptr
 * @return true indicate queue is full
 */
bool fixed_msg_queue_handler_is_full(__in fixed_msg_queue_handler handler_p);

/**
 * clear all msg in queue if msg has not handled.
 * <p>usually it will be fast, but if you stuck in callback, it will effect after the latest callback finished </p>
 * @param handler_p queue handler ptr
 */
void fixed_msg_queue_handler_clear(__in fixed_msg_queue_handler handler_p);

/**
 * destroy queue handler
 * @param handler_p queue handler ptr
 */
void fixed_msg_queue_handler_destroy(__inout fixed_msg_queue_handler *handler_pp);

#ifdef __cplusplus
}
#endif

#endif // LCU_FIXED_MSG_QUEUE_HANDLER_H
