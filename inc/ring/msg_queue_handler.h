#pragma once
#ifndef LCU_MSG_QUEUE_HANDLER_H
#define LCU_MSG_QUEUE_HANDLER_H

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

/**
 * msg prototype
 */
typedef struct 
{
    int what;
    int arg1;
    int arg2;
    int obj_len;
    char obj[0];
} queue_msg_t;

typedef struct __msg_queue_handler *msg_queue_handler;

/**
 * callback to handle msg
 * note: you shouldn't do too much time-consuming operation on here.
 * @param queue_msg_t pointer to popped msg
 * @param user_data user data pointer in msg_queue_handler_create
 */
typedef void (*msg_handler_callback)(queue_msg_t *msg_p, void *user_data);

/**
 * create fixed_queue_handler
 * @param queue_buf_size buffer size of msg queue
 * @param callback handle msg function
 * @return queue handler ptr
 */
msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size, __in msg_handler_callback callback, __in void* callback_userdata);

/**
 * send msg to queue handler
 * @param handler_p queue handler ptr
 * @param msg_p msg ptr
 * @return status. 0 succeed. otherwise failed(see error code details on msg_queue.h)
 */
int msg_queue_handler_send(__in msg_queue_handler handler_p, __in queue_msg_t *msg_p);

/**
 * available push byte size 
 * <Note: this size is NOT completely equal msg size>
 * @param handler_p queue handler ptr
 * @return byte size
 */
uint32_t msg_queue_handler_available_push_bytes(__in msg_queue_handler handler_p);

/**
 * available pop byte size 
 * <Note: this size is NOT completely equal msg size>
 * @param handler_p queue handler ptr
 * @return byte size
 */
uint32_t msg_queue_handler_available_pop_bytes(__in msg_queue_handler handler_p);

/**
 * destroy queue handler
 * @param handler_p queue handler ptr
 */
void msg_queue_handler_destroy(__inout msg_queue_handler *handler_pp);

#ifdef __cplusplus
}
#endif

#endif // LCU_MSG_QUEUE_HANDLER_H
