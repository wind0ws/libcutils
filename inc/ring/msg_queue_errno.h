#pragma once
#ifndef LCU_MSG_QUEUE_ERRNO_H
#define LCU_MSG_QUEUE_ERRNO_H

typedef enum
{
	// thread is started, and ready to pop msg
	MSG_Q_HANDLER_STATUS_READY_TO_GO = 0,
	// thread is about to stop
	MSG_Q_HANDLER_STATUS_ABOUT_TO_STOP,
} msg_q_handler_status_e;

typedef enum
{
	/* success */
	MSG_Q_CODE_SUCCESS = 0,
	/* generic error */
	MSG_Q_CODE_GENERIC_FAIL,
	/* null handle, check your param */
	MSG_Q_CODE_NULL_HANDLE,
	/* msg is not full copied to queue, try pop it again later */
	MSG_Q_CODE_AGAIN,
	/* msg queue empty: can't pop msg */
	MSG_Q_CODE_EMPTY,
	/* msg queue full: can't push msg */
	MSG_Q_CODE_FULL,
	/* the buffer you provide is not enough to hold(pop) msg, you should make buf bigger */
	MSG_Q_CODE_BUF_NOT_ENOUGH,
} msg_q_code_e;

#endif // !LCU_MSG_QUEUE_ERRNO_H
