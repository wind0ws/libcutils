#pragma once
#ifndef __LCU_RINGBUFFER_H
#define __LCU_RINGBUFFER_H

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

#define __RING_LOG_TAG "RingBuf_TAG"
#ifdef _WIN32
//msvc-doesnt-expand-va-args-correctly https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define EXPAND_VA_ARGS( x ) x
//#define F(x, ...) X = x and VA_ARGS = __VA_ARGS__
//#define G(...) EXPAND_VA_ARGS( F(__VA_ARGS__) )

#define __CONSOLE_LOG_NO_NEW_LINE_HELPER(level, ...)  { printf( __VA_ARGS__ ); }
#define __CONSOLE_LOG_NO_NEW_LINE(level, ...)  EXPAND_VA_ARGS( __CONSOLE_LOG_NO_NEW_LINE_HELPER(level, __VA_ARGS__) )
#define __CONSOLE_LOGV_NO_NEW_LINE(...) __CONSOLE_LOG_NO_NEW_LINE(1, ##__VA_ARGS__)
#define __CONSOLE_LOGD_NO_NEW_LINE(...) __CONSOLE_LOG_NO_NEW_LINE(2, ##__VA_ARGS__)
#define __CONSOLE_LOGI_NO_NEW_LINE(...) __CONSOLE_LOG_NO_NEW_LINE(3, ##__VA_ARGS__)
#define __CONSOLE_LOGW_NO_NEW_LINE(...) __CONSOLE_LOG_NO_NEW_LINE(4, ##__VA_ARGS__)
#define __CONSOLE_LOGE_NO_NEW_LINE(...) __CONSOLE_LOG_NO_NEW_LINE(5, ##__VA_ARGS__)

#define __CONSOLE_TLOGV(tag, fmt, ...)  __CONSOLE_LOG_NO_NEW_LINE(1,"[V][%s] " fmt "\r\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGD(tag, fmt, ...)  __CONSOLE_LOG_NO_NEW_LINE(2,"[D][%s] " fmt "\r\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGI(tag, fmt, ...)  __CONSOLE_LOG_NO_NEW_LINE(3,"[I][%s] " fmt "\r\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGW(tag, fmt, ...)  __CONSOLE_LOG_NO_NEW_LINE(4,"[W][%s] " fmt "\r\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGE(tag, fmt, ...)  __CONSOLE_LOG_NO_NEW_LINE(5,"[E][%s] " fmt "\r\n", tag, ##__VA_ARGS__)

#define __LOG_HELPER_V(fmt, ...) __CONSOLE_TLOGV(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_D(fmt, ...) __CONSOLE_TLOGD(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_I(fmt, ...) __CONSOLE_TLOGI(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_W(fmt, ...) __CONSOLE_TLOGW(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_E(fmt, ...) __CONSOLE_TLOGE(__RING_LOG_TAG, fmt, ##__VA_ARGS__)

#elif(defined(__ANDROID__)) //android
#include <android/log.h>
#define __LOG_HELPER_V(fmt,...) __android_log_print(ANDROID_LOG_VERBOSE, __RING_LOG_TAG, fmt, ##__VA_ARGS__);
#define __LOG_HELPER_D(fmt,...) __android_log_print(ANDROID_LOG_DEBUG, __RING_LOG_TAG, fmt, ##__VA_ARGS__);
#define __LOG_HELPER_I(fmt,...) __android_log_print(ANDROID_LOG_INFO, __RING_LOG_TAG, fmt, ##__VA_ARGS__);
#define __LOG_HELPER_W(fmt,...) __android_log_print(ANDROID_LOG_WARN, __RING_LOG_TAG, fmt, ##__VA_ARGS__);
#define __LOG_HELPER_E(fmt,...) __android_log_print(ANDROID_LOG_ERROR, __RING_LOG_TAG, fmt, ##__VA_ARGS__);
#else //unix printf
#define __LOG_HELPER(...) printf(__VA_ARGS__);
#define __LOG_HELPER_V(fmt,...) __LOG_HELPER(__RING_LOG_TAG"[V]"fmt"\r\n", ##__VA_ARGS__)
#define __LOG_HELPER_D(fmt,...) __LOG_HELPER(__RING_LOG_TAG"[D]"fmt"\r\n", ##__VA_ARGS__)
#define __LOG_HELPER_I(fmt,...) __LOG_HELPER(__RING_LOG_TAG"[I]"fmt"\r\n", ##__VA_ARGS__)
#define __LOG_HELPER_W(fmt,...) __LOG_HELPER(__RING_LOG_TAG"[W]"fmt"\r\n", ##__VA_ARGS__)
#define __LOG_HELPER_E(fmt,...) __LOG_HELPER(__RING_LOG_TAG"[E]"fmt"\r\n", ##__VA_ARGS__)
#endif

#define RING_LOGV(fmt,...) __LOG_HELPER_V(fmt, ##__VA_ARGS__);
#define RING_LOGD(fmt,...) __LOG_HELPER_D(fmt, ##__VA_ARGS__);
#define RING_LOGI(fmt,...) __LOG_HELPER_I(fmt, ##__VA_ARGS__);
#define RING_LOGW(fmt,...) __LOG_HELPER_W(fmt, ##__VA_ARGS__);
#define RING_LOGE(fmt,...) __LOG_HELPER_E(fmt, ##__VA_ARGS__);

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct __ring_buffer_t* ring_buf_handle;

	/**
	 * create RingBuffer.
	 * <p>WARN: The ring buffer real size is not always as same as you provided.</p>
	 * @param size RingBuffer size, must be pow of 2. if not, will change size to next pow of it automatically.
	 * @return RingBuffer pointer
	 */
	ring_buf_handle RingBuffer_create(__in uint32_t size);

	/**
	 * destroy RingBuffer and free memory
	 * @param ring_buf_pp RingBuffer pointer's pointer
	 */
	void RingBuffer_destroy(__in ring_buf_handle *ring_buf_pp);

	/**
	 * get current RingBuffer available data size
	 * @param ring_buf_p RingBuffer
	 * @return available read byte size
	 */
	uint32_t RingBuffer_available_read(__in const ring_buf_handle ring_buf_p);

	/**
	 * get current RingBuffer available space to write
	 * @param ring_buf_p RingBuffer
	 * @return available write byte size
	 */
	uint32_t RingBuffer_available_write(__in const ring_buf_handle ring_buf_p);

	/**
	 * indicate RingBuffer whether is empty(no data to read)
	 * @param ring_buf_p RingBuffer
	 * @return true indicate no data to read
	 */
	bool RingBuffer_is_empty(__in const ring_buf_handle ring_buf_p);

	/**
	 * indicate RingBuffer whether is full(no space to write)
	 * @param ring_buf_p RingBuffer
	 * @return true indicate no space to write
	 */
	bool RingBuffer_is_full(__in const ring_buf_handle ring_buf_p);

	/**
	 * copy specified size memory from RingBuffer to target
	 * <p>If current RingBuffer can read data size is little than specified size, then only copy max readable data size to target </p>
	 * @param ring_buf_p RingBuffer
	 * @param target the target pointer to write data
	 * @param size copy specified size memory
	 * @return the real read data size
	 */
	uint32_t RingBuffer_read(__in const ring_buf_handle ring_buf_p, __out void* target, uint32_t size);

	/**
	 * peek(read) specified size memory from RingBuffer to target: this will NOT change read offset.
	 * <p>If current RingBuffer can read data size is little than specified size, then only copy max readable data size to target </p>
	 * @param ring_buf_p RingBuffer
	 * @param target the target pointer to write data
	 * @param size copy specified size memory
	 * @return the real peek(read) data size
	 */
	uint32_t RingBuffer_peek(__in const ring_buf_handle ring_buf_p, __out void* target, uint32_t size);

	/**
	 * discard specified size memory from RingBuffer: behavior like "RingBuffer_read" but NOT copy memory.
	 * <p>If current RingBuffer can read data size is little than specified size, then return the little one </p>
	 * @param ring_buf_p RingBuffer
	 * @param size discard specified size memory
	 * @return the real discard data size
	 */
	uint32_t RingBuffer_discard(__in const ring_buf_handle ring_buf_p, uint32_t size);

	/**
	 * copy specified size memory from source to RingBuffer.
	 * <p>If current RingBuffer can write data size is little than specified size, then only copy max writable data size from source </p>
	 * @param ring_buf_p RingBuffer
	 * @param source the source pointer to read data
	 * @param size copy specified size memory
	 * @return the real wrote data size
	 */
	uint32_t RingBuffer_write(__in const ring_buf_handle ring_buf_p, __in const void* source, uint32_t size);

	/**
	 * current read position of ringbuffer
	 * <p>in most case, you don't need care about this, this function for debug purpose</p>
	 * @return the read position
	 */
	uint32_t RingBuffer_current_read_position(__in const ring_buf_handle ring_buf_p);

	/**
	 * current write position of ringbuffer
     * <p>in most case, you don't need care about this, this function for debug purpose</p>
	 * @return the write position
	*/
	uint32_t RingBuffer_current_write_position(__in const ring_buf_handle ring_buf_p);

	/**
	 * the real buffer size of ring
     * <p>in most case, you don't need care about this, this function for debug purpose</p>
	 * @return the real buffer size
	 */
	uint32_t RingBuffer_real_capacity(__in const ring_buf_handle ring_buf_p);

	/**
	 * clear RingBuffer all data.
	 * <p>WARN: this method is NOT thread safe!!!</p>
	 * @param ring_buffer_p RingBuffer handle
	 */
	void RingBuffer_clear(__in const ring_buf_handle ring_buffer_p);


#ifdef __cplusplus
}
#endif

#endif //__LCU_RINGBUFFER_H