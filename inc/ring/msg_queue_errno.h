#pragma once
#ifndef LCU_MSG_QUEUE_ERRNO_H
#define LCU_MSG_QUEUE_ERRNO_H

typedef enum
{
	/* success */
	MSG_Q_CODE_SUCCESS = 0,
	/* generic error */
	MSG_Q_CODE_GENERIC_FAIL = 1,
	/* null handle, check your param */
	MSG_Q_CODE_NULL_HANDLE,
	/* msg is not full copied to queue, try again later */
	MSG_Q_CODE_AGAIN,
	/* msg queue empty: can't pop msg */
	MSG_Q_CODE_EMPTY,
	/* msg queue full: can't push msg */
	MSG_Q_CODE_FULL,
	/* the buffer you provide is not enough to hold(pop) msg, you should make buf bigger */
	MSG_Q_CODE_BUF_NOT_ENOUGH,
} MSG_Q_CODE;

#endif // !LCU_MSG_QUEUE_ERRNO_H
