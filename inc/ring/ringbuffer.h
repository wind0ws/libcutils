#pragma once
#ifndef LCU_RINGBUFFER_H
#define LCU_RINGBUFFER_H

#include <stdio.h>
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

	typedef struct __ring_buffer_t* ring_buf_handle;

	/**
	 * create RingBuffer.
	 * <p>WARN: The ring buffer real size is not always as same as you provided.</p>
	 * @param buf_size RingBuffer size, must be pow of 2. if not, will change size to next pow of it automatically.
	 * @return RingBuffer pointer
	 */
	ring_buf_handle RingBuffer_create(__in uint32_t buf_size);

	/**
	 * create RingBuffer with you provided memory.
	 * if the memory allocated on heap, you should free it after call RingBuffer_destory
	 * <p>WARN: The ring buffer real size is not always as same as you provided.</p>
	 * @param buf_size RingBuffer size, must be pow of 2. if not, will change size to previous pow of it automatically.
	 * @return RingBuffer pointer
	 */
	ring_buf_handle RingBuffer_create_with_mem(__in char* buf, __in uint32_t buf_size);

	/**
	 * destroy RingBuffer and free memory
	 * @param ring_buf_pp RingBuffer pointer's pointer
	 */
	void RingBuffer_destroy(__inout ring_buf_handle *ring_buf_pp);

	/**
	 * get current RingBuffer available data size
	 * @param ring_buf_p RingBuffer
	 * @return available read byte size
	 */
	uint32_t RingBuffer_available_read(__in ring_buf_handle ring_buf_p);

	/**
	 * get current RingBuffer available space to write
	 * @param ring_buf_p RingBuffer
	 * @return available write byte size
	 */
	uint32_t RingBuffer_available_write(__in ring_buf_handle ring_buf_p);

	/**
	 * indicate RingBuffer whether is empty(no data to read)
	 * @param ring_buf_p RingBuffer
	 * @return true indicate no data to read
	 */
	bool RingBuffer_is_empty(__in ring_buf_handle ring_buf_p);

	/**
	 * indicate RingBuffer whether is full(no space to write)
	 * @param ring_buf_p RingBuffer
	 * @return true indicate no space to write
	 */
	bool RingBuffer_is_full(__in ring_buf_handle ring_buf_p);

	/**
	 * copy specified size memory from RingBuffer to target
	 * <p>If current RingBuffer can read data size is little than specified size, then only copy max readable data size to target </p>
	 * @param ring_buf_p RingBuffer
	 * @param target the target pointer to write data
	 * @param size copy specified size memory
	 * @return the real read data size
	 */
	uint32_t RingBuffer_read(__in ring_buf_handle ring_buf_p, __out void* target, __in uint32_t size);

	/**
	 * peek(read) specified size memory from RingBuffer to target: this will NOT change read offset.
	 * <p>If current RingBuffer can read data size is little than specified size, then only copy max readable data size to target </p>
	 * @param ring_buf_p RingBuffer
	 * @param target the target pointer to write data
	 * @param size copy specified size memory
	 * @return the real peek(read) data size
	 */
	uint32_t RingBuffer_peek(__in ring_buf_handle ring_buf_p, __out void* target, __in uint32_t size);

	/**
	 * discard specified size memory from RingBuffer: behavior like "RingBuffer_read" but NOT copy memory.
	 * <p>If current RingBuffer can read data size is little than specified size, then return the little one </p>
	 * @param ring_buf_p RingBuffer
	 * @param size discard specified size memory
	 * @return the real discard data size
	 */
	uint32_t RingBuffer_discard(__in ring_buf_handle ring_buf_p, __in uint32_t size);

	/**
	 * copy specified size memory from source to RingBuffer.
	 * <p>If current RingBuffer can write data size is little than specified size, then only copy max writable data size from source </p>
	 * @param ring_buf_p RingBuffer
	 * @param source the source pointer to read data
	 * @param size copy specified size memory
	 * @return the real wrote data size
	 */
	uint32_t RingBuffer_write(__in ring_buf_handle ring_buf_p, __in const void* source, __in uint32_t size);

	/**
	 * current read position of ringbuffer
	 * <p>in most case, you don't need care about this, this function for debug purpose</p>
	 * @return the read position
	 */
	uint32_t RingBuffer_current_read_position(__in ring_buf_handle ring_buf_p);

	/**
	 * current write position of ringbuffer
     * <p>in most case, you don't need care about this, this function for debug purpose</p>
	 * @return the write position
	*/
	uint32_t RingBuffer_current_write_position(__in ring_buf_handle ring_buf_p);

	/**
	 * the real buffer size of ring
     * <p>in most case, you don't need care about this, this function for debug purpose</p>
	 * @return the real buffer size
	 */
	uint32_t RingBuffer_real_capacity(__in ring_buf_handle ring_buf_p);

	/**
	 * clear RingBuffer all data.
	 * <p>WARN: this method is NOT thread safe!!!</p>
	 * @param ring_buffer_p RingBuffer handle
	 */
	void RingBuffer_clear(__in ring_buf_handle ring_buffer_p);


#ifdef __cplusplus
}
#endif

#endif // LCU_RINGBUFFER_H
