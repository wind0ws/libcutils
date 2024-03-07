#pragma once
#ifndef LCU_RING_BUF_H
#define LCU_RING_BUF_H

#include <stddef.h>   /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct __ring_buf_t* ring_handle;

	/**
	 * @brief create ring_buf with provided pbuf.
     * 
	 * you maybe need free pbuf by yourself 
     * after ring_buf_destroy() if your pbuf is alloced on heap.
     * 
	 * @param[in] pbuf the buf to hold ring data.
	 * @param[in] buf_size the pbuf memory size.
     * 
	 * @return ring_buf handle
	 */
	ring_handle ring_buf_create_with_mem(void* pbuf, const size_t buf_size);

	/**
	 * @brief create ring_buf.
     * 
	 * @param[in] size ring_buf size. can hold (size - 1) byte.
     * 
	 * @return ring_buf handle
	 */
	ring_handle ring_buf_create(const size_t size);

    /**
     * @brief The data count available to read.
     * 
     * @param[in] handle ring_buf handle
     * 
     * @return size of current available data.
     */
    size_t ring_buf_available_read(ring_handle handle);

    /**
     * @brief The space count available to write.
     * 
     * @param[in] handle ring_buf handle
     * 
     * @return size of current available space.
     */
    size_t ring_buf_available_write(ring_handle handle);

    /**
     * @brief read data from ring_buf.
     * 
     * @param[in] handle ring_buf handle
     * @param[out] target copy read data to this pointer.
     * @param[in] len the length you want to read.
     * 
     * @return size of real read.
     */
    size_t ring_buf_read(ring_handle handle, void* target, size_t len);

    /**
     * @brief peek(read) data from ring_buf.

     * this method is seems like ring_buf_read(), 
     * but it not actually increase ring buffer read offset.
     * 
     * @param[in] handle ring_buf handle
     * @param[out] target copy read data to this pointer.
     * @param[in] len the length you want to read.
     * 
     * @return size of real read.
     */
    size_t ring_buf_peek(ring_handle handle, void* target, size_t len);

    /**
	 * @brief discard(read) data from ring_buf.
     * 
     * it is no memcpy version of ring_buf_read(). 
     * 
	 * @param[in] handle ring_buf handle
	 * @param[in] len the length you want to read.
     * 
	 * @return size of real discard(read).
     */
    size_t ring_buf_discard(ring_handle handle, size_t len);

    /**
     * @brief write data to ring_buf
     * 
     * @param[in] handle ring_buf handle
     * @param[in] source copy data of this pointer to ring_buf.
     * @param[in] len the length you want to write.
     * 
     * @return size of real write.
     */
    size_t ring_buf_write(ring_handle handle, void* source, size_t len);

    /**
     * @brief clear ring_buf data.
     * 
     * WARN: you shouldn't call clear while calling read/write, 
     * otherwise it will have thread safe issue.
     * 
     * @param[in] handle ring_buf handle.
     */
    void ring_buf_clear(ring_handle handle);

    /**
     * @brief destroy ring_buf and release memory.
     * 
     * maybe you need free pbuf by yourself after this destroy call 
     * if you provided allocated pbuf.
     * 
     * @param[in,out] handle_p ring_buf handle's pointer.
     */
    void ring_buf_destroy(ring_handle* handle_p);

#ifdef __cplusplus
}
#endif

#endif // !LCU_RING_BUF_H
