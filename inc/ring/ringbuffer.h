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

	typedef struct __ring_buffer_t* ring_buffer_handle;

	/**
	 * @brief create RingBuffer.
	 * WARN: The ring buffer real size is not always as same as you provided.
	 *
	 * @param[in] buf_size RingBuffer size, must be pow of 2. 
	 *                     if not, will change size to next pow of it automatically.
	 * 
	 * @return RingBuffer pointer, NULL if fail 
	 */
	ring_buffer_handle RingBuffer_create(__in uint32_t buf_size);

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
	ring_buffer_handle RingBuffer_create_with_mem(__in void* buf, __in uint32_t buf_size);

	/**
	 * @brief destroy RingBuffer and free memory.
	 * 
	 * @param[in,out] ring_handle_p RingBuffer pointer's pointer
	 */
	void RingBuffer_destroy(__inout ring_buffer_handle *ring_handle_p);

	/**
	 * @brief get current RingBuffer available data size.
	 * 
	 * @param[in] ring_handle RingBuffer
	 * 
	 * @return available read byte size
	 */
	uint32_t RingBuffer_available_read(__in ring_buffer_handle ring_handle);

	/**
	 * @brief get current RingBuffer available space to write.
	 * 
	 * @param[in] ring_handle RingBuffer
	 * 
	 * @return available write byte size
	 */
	uint32_t RingBuffer_available_write(__in ring_buffer_handle ring_handle);

	/**
	 * @brief indicate RingBuffer whether is empty(no data to read).
	 * 
	 * @param[in] ring_handle RingBuffer
	 * 
	 * @return true indicate no data to read
	 */
	bool RingBuffer_is_empty(__in ring_buffer_handle ring_handle);

	/**
	 * @brief indicate RingBuffer whether is full(no space to write)
	 * 
	 * @param[in] ring_handle RingBuffer
	 * 
	 * @return true indicate no space to write
	 */
	bool RingBuffer_is_full(__in ring_buffer_handle ring_handle);

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
	uint32_t RingBuffer_read(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size);

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
	uint32_t RingBuffer_peek(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size);

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
	uint32_t RingBuffer_discard(__in ring_buffer_handle ring_handle, __in uint32_t size);

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
	uint32_t RingBuffer_write(__in ring_buffer_handle ring_handle, __in const void* source, __in uint32_t size);

	/**
	 * @brief current read position of ring buffer.
	 * in most cases, you don't need care about this, this function for debug purpose.
	 * 
	 * @param[in] ring_handle RingBuffer
	 * 
	 * @return the read position
	 */
	uint32_t RingBuffer_current_read_position(__in ring_buffer_handle ring_handle);

	/**
	 * @brief current write position of ring buffer.
     * in most cases, you don't need care about this, this function for debug purpose.
	 * 
	 * @param[in] ring_handle RingBuffer
	 * 
	 * @return the write position
	 */
	uint32_t RingBuffer_current_write_position(__in ring_buffer_handle ring_handle);

	/**
	 * @briefthe real buffer size of ring.
     * in most cases, you don't need care about this, this function for debug purpose.
	 * 
	 * @param[in] ring_handle RingBuffer
	 * 
	 * @return the real buffer size
	 */
	uint32_t RingBuffer_real_capacity(__in ring_buffer_handle ring_handle);

	/**
	 * @brief clear RingBuffer all data.
	 * WARN: this method is NOT thread safe!!! 
	 * call this if you make sure that RingBuffer is not in read/write state.
	 * 
	 * @param[in] ring_handle RingBuffer
	 */
	void RingBuffer_clear(__in ring_buffer_handle ring_handle);


#ifdef __cplusplus
}
#endif

#endif // !LCU_RINGBUFFER_H
