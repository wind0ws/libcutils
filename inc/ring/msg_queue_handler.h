#pragma once
#ifndef LCU_MSG_QUEUE_HANDLER_H
#define LCU_MSG_QUEUE_HANDLER_H

#include <stdint.h>   /* for uint32_t */
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

typedef struct _msg_queue_handler_s *msg_queue_handler;

/**
 * callback prototype that handle msg
 * note: you shouldn't do too much time-consuming operation on here.
 * @param queue_msg_t pointer to popped msg. do not free this msg memory.
 * @param user_data user data pointer that you passed in msg_queue_handler_create
 * @return 0 for normal status, otherwise will break the handler queue
 */
typedef int (*msg_handler_callback_t)(queue_msg_t *msg_p, void *user_data);

/**
 * create fixed_queue_handler
 * @param queue_buf_size buffer size of msg queue
 * @param callback handle msg function
 * @param callback_userdata user_data will pass in callback
 * @return queue handler ptr
 */
msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size, 
    __in msg_handler_callback_t callback, __in void* callback_userdata);

/**
 * send msg to queue handler
 * note: if you send msg on multi-thread, you should lock this method.
 * @param handler queue handler ptr
 * @param msg_p msg ptr
 * @return status. 0 succeed. otherwise failed(see error code details on msg_queue_errno.h)
 */
MSG_Q_CODE msg_queue_handler_send(__in msg_queue_handler handler, __in queue_msg_t *msg_p);

/**
 * available push byte size 
 * <Note: this is byte size is NOT completely equal msg count>
 * @param handler queue handler ptr
 * @return byte size
 */
uint32_t msg_queue_handler_available_push_bytes(__in msg_queue_handler handler);

/**
 * available pop byte size 
 * <Note: this is byte size is NOT completely equal msg count>
 * @param handler queue handler ptr
 * @return byte size
 */
uint32_t msg_queue_handler_available_pop_bytes(__in msg_queue_handler handler);

/**
 * destroy queue handler
 * @param handler queue handler ptr
 */
void msg_queue_handler_destroy(__inout msg_queue_handler *handler_p);

#ifdef __cplusplus
}
#endif

#endif // LCU_MSG_QUEUE_HANDLER_H
