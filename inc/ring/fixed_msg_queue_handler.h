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

#ifdef __cplusplus
extern "C" {
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

/**
 * callback prototype of handle msg
 */
typedef void (*fixed_msg_handler_callback_t)(fixed_msg_t *msg_p, void *user_data);

/**
 * create fixed_queue_handler
 * 
 * @param max_msg_capacity max capacity
 * @param callback handle msg function
 * @return queue handler ptr
 */
fixed_msg_queue_handler fixed_msg_queue_handler_create(__in uint32_t max_msg_capacity,
	__in fixed_msg_handler_callback_t callback, __in void* callback_userdata);

/**
 * push msg at tail of queue handler,
 * you should lock this if you call it on multi-thread
 * 
 * @param handler: queue handler ptr
 * @param msg_p: msg ptr
 * @return 0 succeed. otherwise failed
 */
MSG_Q_CODE fixed_msg_queue_handler_push(__in fixed_msg_queue_handler handler, __in fixed_msg_t *msg_p);

/**
 * current available push msg amount
 * 
 * @param handler queue handler ptr
 * @return amount
 */
uint32_t fixed_msg_queue_handler_available_push_amount(__in fixed_msg_queue_handler handler);

/**
 * current msg amount in queue handler
 * 
 * @param handler queue handler ptr
 * @return amount
 */
uint32_t fixed_msg_queue_handler_available_pop_amount(__in fixed_msg_queue_handler handler);

/**
 * if current msg amount in queue handler is zero
 * 
 * @param handler queue handler ptr
 * @return true indicate for empty.
 */
bool fixed_msg_queue_handler_is_empty(__in fixed_msg_queue_handler handler);

/**
 * if current can send msg max amount is zero.
 * 
 * @param handler queue handler ptr
 * @return true indicate queue is full
 */
bool fixed_msg_queue_handler_is_full(__in fixed_msg_queue_handler handler);

/**
 * clear all msg in queue if msg has not handled.
 * <p>usually it will be fast, but if you stuck in callback, it will effect after the latest callback finished </p>
 * 
 * @param handler queue handler ptr
 */
void fixed_msg_queue_handler_clear(__in fixed_msg_queue_handler handler);

/**
 * destroy queue handler
 * 
 * @param handler queue handler ptr
 */
void fixed_msg_queue_handler_destroy(__inout fixed_msg_queue_handler *handler_p);

#ifdef __cplusplus
}
#endif

#endif // !LCU_FIXED_MSG_QUEUE_HANDLER_H
