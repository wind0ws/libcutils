#pragma once
#ifndef LCU_RINGBUFFER_H
#define LCU_RINGBUFFER_H

#include <stdbool.h>
#include <stdint.h>

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
#ifndef __out_opt
#define __out_opt
#endif
#ifndef __inout_opt
#define __inout_opt
#endif

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _ring_buffer_t* ring_buffer_handle;

	/**
	 * @brief create ring_buffer.
	 * WARN: The ring buffer real size is not always as same as you provided.
	 *
	 * @param[in] buf_size ring_buffer size, must be pow of 2.
	 *                     if not, will change size to next pow of it automatically.
	 *
	 * @return ring_buffer pointer, NULL if fail
	 */
	ring_buffer_handle ring_buffer_create(__in uint32_t buf_size);

	/**
	 * @brief create RingBuffer with you provided memory.
	 * if the memory allocated on heap, you should free it after call RingBuffer_destory(hdl).
	 * WARN: The ring buffer real size is not always as same as you provided.
	 *
	 * @param[in] buf_size RingBuffer size, must be pow of 2.
	 *                     if not, will change size to previous pow of it automatically.
	 *
	 * @return RingBuffer pointer
	 */
	ring_buffer_handle ring_buffer_create_with_mem(__in void* buf, __in uint32_t buf_size);

	/**
	 * @brief destroy RingBuffer and free memory.
	 *
	 * @param[in,out] ring_handle_p RingBuffer pointer's pointer
	 */
	void ring_buffer_destroy(__inout ring_buffer_handle* ring_handle_p);

	/**
	 * @brief get current RingBuffer available data size.
	 *
	 * @param[in] ring_handle RingBuffer
	 *
	 * @return available read byte size
	 */
	uint32_t ring_buffer_available_read(__in ring_buffer_handle ring_handle);

	/**
	 * @brief get current RingBuffer available space to write.
	 *
	 * @param[in] ring_handle RingBuffer
	 *
	 * @return available write byte size
	 */
	uint32_t ring_buffer_available_write(__in ring_buffer_handle ring_handle);

	/**
	 * @brief indicate RingBuffer whether is empty(no data to read).
	 *
	 * @param[in] ring_handle RingBuffer
	 *
	 * @return true indicate no data to read
	 */
	bool ring_buffer_is_empty(__in ring_buffer_handle ring_handle);

	/**
	 * @brief indicate RingBuffer whether is full(no space to write)
	 *
	 * @param[in] ring_handle RingBuffer
	 *
	 * @return true indicate no space to write
	 */
	bool ring_buffer_is_full(__in ring_buffer_handle ring_handle);

	/**
	 * @brief copy specified size memory from RingBuffer to target.
	 * If current RingBuffer can read data size is little than specified size,
	 * then only copy max readable data size to target
	 *
	 * @param[in]  ring_handle RingBuffer
	 * @param[out] target the target pointer to write data
	 * @param[in]  size copy specified size memory
	 *
	 * @return the real read data size
	 */
	uint32_t ring_buffer_read(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size);

	/**
	 * @brief just like RingBuffer_read, but skip read with read_offset.
	 *        example: you know your queue data struct size is 1024, and you just want to read out last 256 to target.
	 *                 you can write code like this:
	 *                 char buf[256];
	 *                 RingBuffer_read_with_offset(handle, 1024U - 256U, target, 256U);
	 */
	uint32_t ring_buffer_read_with_offset(__in ring_buffer_handle ring_handle,
		__in uint32_t read_offset, __out void* target, __in uint32_t size);

	/**
	 * @brief peek(read) specified size memory from RingBuffer to target: this will NOT change read offset.
	 * If current RingBuffer can read data size is little than specified size,
	 * then only copy max readable data size to target.
	 *
	 * @param[in]  ring_handle RingBuffer
	 * @param[out] target the target pointer to write data
	 * @param[in]  size copy specified size memory
	 *
	 * @return the real peek(read) data size
	 */
	uint32_t ring_buffer_peek(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size);

	/**
	 * @brief just like RingBuffer_peek, but skip read with read_offset.
	 *        example: you know your queue data struct size is 1024, and you just want to read out last 256 to target.
	 *                 you can write code like this:
	 *                 char buf[256];
	 *                 RingBuffer_peek_with_offset(handle, 1024U - 256U, target, 256U);
	 */
	uint32_t ring_buffer_peek_with_offset(__in ring_buffer_handle ring_handle,
		__in uint32_t peek_offset, __out void* target, __in uint32_t size);

	/**
	 * @brief discard specified size memory from RingBuffer:
	 * behavior like "RingBuffer_read" but NOT copy memory.
	 * If current RingBuffer can read data size is little than specified size,
	 * then return the little one.
	 *
	 * @param[in] ring_handle RingBuffer
	 * @param[in] size discard specified size memory
	 *
	 * @return the real discard data size
	 */
	uint32_t ring_buffer_discard(__in ring_buffer_handle ring_handle, __in uint32_t size);

	/**
	 * @brief copy specified size memory from source to RingBuffer.
	 * If current RingBuffer can write data size is little than specified size,
	 * then only copy max writable data size from source.
	 *
	 * @param[in] ring_handle RingBuffer
	 * @param[in] source the source pointer to read data
	 * @param[in] size copy specified size memory
	 *
	 * @return the real wrote data size
	 */
	uint32_t ring_buffer_write(__in ring_buffer_handle ring_handle, __in const void* source, __in uint32_t size);

	/**
	 * @brief current read position of ring buffer.
	 * in most cases, you don't need care about this, this function for debug purpose.
	 *
	 * @param[in] ring_handle RingBuffer
	 *
	 * @return the read position
	 */
	uint32_t ring_buffer_current_read_position(__in ring_buffer_handle ring_handle);

	/**
	 * @brief current write position of ring buffer.
	 * in most cases, you don't need care about this, this function for debug purpose.
	 *
	 * @param[in] ring_handle RingBuffer
	 *
	 * @return the write position
	 */
	uint32_t ring_buffer_current_write_position(__in ring_buffer_handle ring_handle);

	/**
	 * @briefthe real buffer size of ring.
	 * in most cases, you don't need care about this, this function for debug purpose.
	 *
	 * @param[in] ring_handle RingBuffer
	 *
	 * @return the real buffer size
	 */
	uint32_t ring_buffer_real_capacity(__in ring_buffer_handle ring_handle);

	/**
	 * @brief clear RingBuffer all data.
	 * WARN: this method is NOT thread safe!!!
	 * call this if you make sure that RingBuffer is not in read/write state.
	 *
	 * @param[in] ring_handle RingBuffer
	 */
	void ring_buffer_clear(__in ring_buffer_handle ring_handle);


#ifdef __cplusplus
}
#endif

#endif // !LCU_RINGBUFFER_H
