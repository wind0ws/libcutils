#pragma once
#ifndef LCU_AUTOCOVER_BUFFER_H
#define LCU_AUTOCOVER_BUFFER_H

#include <stdint.h>	 /* for uint32_t */

#ifdef _WIN32
#include <sal.h> /* for in/out param */
#endif // _WIN32

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
#ifndef __success
#define __success(expr) 
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * @brief error code for auto_cover_buf.
	 * 
	 * error code must be negative number,
	 * because positive number as normal read/write bytes.
	 */
	typedef enum
	{
		/* invalid handle */
		AUTO_COVER_BUF_ERR_INVALID_HANDLE = -1,
		/* invalid param value */
		AUTO_COVER_BUF_ERR_INVALID_PARAM = -2,
		/* the data you want to read is already covered */
		AUTO_COVER_BUF_ERR_DATA_COVERED = -3,
		/* the data you wan to read is not enough, maybe read it later(try again) */
		AUTO_COVER_BUF_ERR_DATA_NOT_ENOUGH = -4,
	} auto_cover_buf_error_code;

	typedef struct auto_cover_buf_lock_s
	{
		void* arg; /* Argument to be passed to acquire and release function pointers */
		int (*acquire)(void* arg); /* Function pointer to acquire a lock */
		int (*release)(void* arg); /* Function pointer to release a lock */
	} auto_cover_buf_lock_t;

	typedef struct _auto_cover_buf* auto_cover_buf_handle;

	/**
	 * @brief create auto_cover_buf.
	 *
	 * @param[in] capacity_size the size of auto cover buf size.
	 * @param[in] buf_lock_p lock auto_cover_buf instance in read/write.
	 *                       if you put NULL, you should protect read/write function by yourself!
	 *                       if you read/write on one thread, here is fine to pass NULL.
	 *
	 * @return auto_cover_buf_handle
	 */
	auto_cover_buf_handle auto_cover_buf_create(__in uint32_t capacity_size,
		__in_opt auto_cover_buf_lock_t* buf_lock_p);

	/**
	 * @brief available read data size start from auto_cover_buf's read_pos
	 *
	 * @param[in] buf_handle auto_cover_buf_handle
	 * @param[in] read_pos offset of read buffer
	 *
	 * @return available read size
	 * 	       if return negative number that indicate error (auto_cover_buf_error_code)
	 */
	int auto_cover_buf_available_read(__in const auto_cover_buf_handle buf_handle,
		__in uint32_t read_pos);

	/**
	 * @brief read data start from auto_cover_buf's read_pos
	 *
	 * @param[in]  buf_handle auto_cover_buf_handle
	 * @param[in]  read_pos offset of read buffer
	 * @param[out] target copy read out data to this
	 * @param[in]  req_read_len request read out data length
	 *
	 * @return real read data size,
	 *         if return negative number that indicate error (auto_cover_buf_error_code)
	 */
	int auto_cover_buf_read(__in const auto_cover_buf_handle buf_handle,
		__in uint32_t read_pos, __out void* target, __in uint32_t req_read_len);

	/**
	 * @brief write data to tail of auto_cover_buf
	 *
	 * @param[in] source copy this to auto_cover buffer
	 * @param[in] write_len the length you want to write to buffer
	 *
	 * @return real write data size,
	 *         if return negative number that indicate error (auto_cover_buf_error_code)
	 */
	int auto_cover_buf_write(__in const auto_cover_buf_handle buf_handle,
		__in const void* source, __in uint32_t write_len);

	/**
	 * @brief destroy auto_cover_buf_handle
	 *
	 * note: after destroy, maybe you need cleanup lock, 
	 *       which you passed it in create function.
	 *
	 * @param[in,out] buf_handle_p the pointer of auto_cover_buf_handle
	 */
	void auto_cover_buf_destroy(__inout auto_cover_buf_handle* buf_handle_p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !LCU_AUTOCOVER_BUFFER_H
