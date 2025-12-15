#pragma once
#ifndef LCU_MSG_QUEUE_HANDLER_H
#define LCU_MSG_QUEUE_HANDLER_H

#include <stdint.h> /* for uint32_t */
#include "ring/msg_queue_errno.h"

#ifdef _WIN32
#include <sal.h> /* for in/out param */
#endif			 // _WIN32

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

/*
 * 特殊情况说明: (一般情况很难触发这个条件):
 *  msg_queue_handler 保留的 what 值: 用于内部标识外部的内存封装的大消息, 用户禁止使用该 what 值发送普通消息 .
 *   当 push 时如果检测到 obj_len 大于配置的阈值(threshold_mem_size_for_alloc_obj),
 *   且 (MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT == what),
 *   且消息的 obj_len 恰好等于 sizeof(big_msg_obj_ref_ex_t) 时, 将在 push 消息时失败.
 *
 * 开发者 最简单的规避方式就是 what 值不使用 MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT.
 */
#ifndef MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT
#define MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT INT32_MIN
#endif // !MSG_QUEUE_HANDLER_WHAT_BIG_INDIRECT

typedef enum
{
	MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY = 0,	/* 正常销毁: 退出阶段的队列消息将被丢弃 */
	MSG_Q_HANDLER_DESTROY_FLAGS_GRACEFULLY = 1, /* 优雅销毁: 退出阶段的队列消息仍交给用户处理 */
} msg_q_handler_destroy_flags_e;

#ifdef __cplusplus
extern "C"
{
#endif

// queue msg header member
#define _DECLARE_QUEUE_MSG_HEADER_MEMBER \
	int what;	 /* msg type  */         \
	int arg1;	 /* user arg1 */         \
	int arg2;	 /* user arg2 */         \
	int obj_len; /* length of obj */

	/**
	 * msg header prototype.
	 */
	typedef struct
	{
		_DECLARE_QUEUE_MSG_HEADER_MEMBER
	} queue_msg_header_t;

	/**
	 * msg prototype 
	 * with flexible array member.
	 */
	typedef struct
	{
		_DECLARE_QUEUE_MSG_HEADER_MEMBER
		char obj[0]; /* must be last field of struct */
	} queue_msg_t;

	typedef struct msg_queue_handler_s *msg_queue_handler;

	/* init param for msg_queue_handler_create */
	typedef struct
	{
		struct
		{
			/* user_data that will pass in callback function */
			void *user_data;

			/**
			 * @brief callback prototype of handle msg.
			 *
			 * note: you shouldn't do too much time-consuming operation on here.
			 *
			 * @param[in] msg_p        pointer to popped msg. do not freed this msg memory.
			 * @param[in] user_data    user data pointer that you passed in init_param
			 *
			 * @return 0 for normal status, otherwise will break the handler queue
			 */
			int (*fn_handle_msg)(queue_msg_t *msg_p, void *user_data);

			/**
			 * @brief callback prototype of notify handler status changed
			 *
			 * note: do NOT call any function of msg_queue_handler on this function,
			 *       otherwise may cause stuck on thread of msg_queue_handler.
			 *
			 * @param[in] status   	   status of current msg_queue_handler
			 * @param[in] user_data    user data pointer that you passed in init_param
			 */
			void (*fn_on_status_changed)(msg_q_handler_status_e status, void *user_data);
		} callback;

		struct
		{
			/**
			 * @brief threshold memory size for alloc object,
			 *        if data size > threshold, then alloc object, else copy data to internal ring buffer.
			 *
			 *        if this set to zero, which means not allow alloc object,
			 *        so if msg size big than left ring buffer size, then return error.
			 */
			uint32_t threshold_mem_size_for_alloc_obj;
		} cfg;
	} msg_queue_handler_init_param_t;

	/**
	 * @brief create msg_queue_handler.
	 *
	 * @param[in] queue_buf_size    total size of memory to hold msg
	 * @param[in] param_p           pointer of msg_queue_handler_init_param_t.
	 *
	 * @return queue handler ptr if success, otherwise return null
	 */
	msg_queue_handler msg_queue_handler_create(__in uint32_t queue_buf_size,
											   __in msg_queue_handler_init_param_t *param_p);

	/**
	 * @brief push msg at tail of queue handler.
	 *
	 * note: if you send msg on multi-thread, you should lock this method.
	 *
	 * @param[in]  handler  queue handler ptr
	 * @param[in]  msg_p    msg ptr, will copy it's memory to queue
	 *
	 * @return status. 0 succeed. otherwise failed(see error code details on msg_queue_errno.h)
	 */
	msg_q_code_e msg_queue_handler_push(__in msg_queue_handler handler, __in queue_msg_t *msg_p);

	/**
	 * @brief available push byte size.
	 *
	 * note: this is byte size which NOT completely equal msg count.
	 *
	 * @param[in] handler   queue handler ptr
	 *
	 * @return byte size
	 */
	uint32_t msg_queue_handler_available_push_bytes(__in msg_queue_handler handler);

	/**
	 * @brief available pop byte size.
	 *
	 * note: this is byte size which NOT completely equal msg count.
	 *
	 * @param[in] handler   queue handler ptr
	 *
	 * @return byte size
	 */
	uint32_t msg_queue_handler_available_pop_bytes(__in msg_queue_handler handler);

	/**
	 * @brief graceful destroy queue handler.
	 *
	 * @param[in,out] handler   pointer of queue handler pointer
	 * @param[in]     flags     destroy flags. see msg_q_handler_destroy_flags_e
	 */
	void msg_queue_handler_destroy(__inout msg_queue_handler *handler_p, __in int flags);

#ifdef __cplusplus
}
#endif

#endif // !LCU_MSG_QUEUE_HANDLER_H
