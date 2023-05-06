#pragma once
#ifndef LCU_AUTOCOVER_BUFFER_H
#define LCU_AUTOCOVER_BUFFER_H

#include <stdint.h>	 /* for uint32_t */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef enum 
	{
		/* invalid handle */
		AUTO_COVER_BUF_ERR_INVALID_HANDLE = -1,
		/* invalid param value */
		AUTO_COVER_BUF_ERR_INVALID_PARAM = -2,
		/* the data you want to read is already covered */
		AUTO_COVER_BUF_ERR_DATA_COVERED = -3,
		/* the data you wan to read is not enough, maybe next time call is ready(try again) */
		AUTO_COVER_BUF_ERROR_DATA_NOT_ENOUGH = -4
	}auto_cover_buf_error_code;

	typedef struct auto_cover_buf_lock 
	{
		void* arg; /**< Argument to be passed to acquire and release function pointers */
		int (*acquire)(void* arg); /**< Function pointer to acquire a lock */
		int (*release)(void* arg); /**< Function pointer to release a lock */
	} auto_cover_buf_lock_t;

	typedef struct __auto_cover_buf* auto_cover_buf_handle;

	/**
	 * create auto_cover_buf.
	 * @param capacity_size the size of auto cover buf size.
	 * @param buf_lock_p lock auto_cover_buf instance in read/write
	 *                   if you put NULL, you should protect read/write function!
	 * @return auto_cover_buf_handle
	 */
	auto_cover_buf_handle auto_cover_buf_create(uint32_t capacity_size, auto_cover_buf_lock_t *buf_lock_p);

	/**
	 * available read data size start from auto_cover_buf's read_pos
	 * @param buf_handle auto_cover_buf_handle
	 * @param read_pos offset of read buffer
	 * @return available read size
	 */
	int auto_cover_buf_available_read(const auto_cover_buf_handle buf_handle, uint32_t read_pos);

	/**
	 * read data start from auto_cover_buf's read_pos
	 * @param buf_handle auto_cover_buf_handle
	 * @param read_pos offset of read buffer
	 * @param target copy read out data to this
	 * @param req_read_len request read out data length
	 * @return real read data size, if error may return auto_cover_buf_error_code
	 */
	int auto_cover_buf_read(const auto_cover_buf_handle buf_handle, uint32_t read_pos, char* target, uint32_t req_read_len);

	/**
	 * write data to auto_cover_buf
	 * @param source copy this to auto_cover buffer
	 * @param write_len the length you want to write to buffer
	 * @return real write data size, if error may return auto_cover_buf_error_code
	 */
	int auto_cover_buf_write(const auto_cover_buf_handle buf_handle, char* source, uint32_t write_len);

	/**
	 * destroy auto_cover_buf_handle
	 * @param buf_handle_p the pointer of auto_cover_buf_handle
	 */
	void auto_cover_buf_destroy(auto_cover_buf_handle* buf_handle_p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !LCU_AUTOCOVER_BUFFER_H
