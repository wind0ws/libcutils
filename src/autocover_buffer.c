#include "autocover_buffer.h"
#include "ringbuffer.h"
#include "common_macro.h"
#include <malloc.h>
#include <string.h>

struct __auto_cover_buf
{
	/* ring buffer handle*/
	ring_buf_handle ring;
	/* ring buffer real size */
	uint32_t ring_buffer_size;
	/* auto_cover_buf lock struct */
	auto_cover_buf_lock_t buf_lock;
};

#define AUTOCOVER_LOCK(buf_handle) 	if (buf_handle->buf_lock.acquire)\
{\
	buf_handle->buf_lock.acquire(buf_handle->buf_lock.arg);\
};
#define AUTOCOVER_UNLOCK(buf_handle) 	if (buf_handle->buf_lock.release)\
{\
	buf_handle->buf_lock.release(buf_handle->buf_lock.arg);\
};


auto_cover_buf_handle auto_cover_buf_create(uint32_t capacity_size, auto_cover_buf_lock_t *buf_lock_p)
{
	auto_cover_buf_handle buf_handle = (auto_cover_buf_handle)malloc(sizeof(struct __auto_cover_buf));
	ASSERT_RET_VALUE(buf_handle, NULL);
	memset(buf_handle, 0, sizeof(struct __auto_cover_buf));
	if (buf_lock_p)
	{
		memcpy(&buf_handle->buf_lock, buf_lock_p, sizeof(auto_cover_buf_lock_t));
	}
	buf_handle->ring = RingBuffer_create(capacity_size);
	ASSERT_ABORT(buf_handle->ring);
	buf_handle->ring_buffer_size = RingBuffer_real_capacity(buf_handle->ring);
	return buf_handle;
}

/**
 * read cover buf  start from read_pos
 * @return if return value below 0, that means the data you want to read start from read_pos is already covered.
 */
static int auto_cover_buf_internal_read(const auto_cover_buf_handle buf_handle, uint32_t read_pos, char* target, uint32_t request_read_len)
{
	ASSERT_RET_VALUE(buf_handle, AUTO_COVER_BUF_ERR_INVALID_HANDLE);
	AUTOCOVER_LOCK(buf_handle);

	//default status is the data you want to read is been covered.
	int real_can_read_len = AUTO_COVER_BUF_ERR_DATA_COVERED;
	uint32_t cover_read_off = read_pos & (buf_handle->ring_buffer_size - 1);
	uint32_t ring_available_data = RingBuffer_available_read(buf_handle->ring);
	uint32_t ring_read_off = RingBuffer_current_read_position(buf_handle->ring) & (buf_handle->ring_buffer_size - 1);
	uint32_t ring_write_off = RingBuffer_current_write_position(buf_handle->ring) & (buf_handle->ring_buffer_size - 1);
	if (ring_write_off - ring_read_off == ring_available_data)
	{
		//current ring read from "out --> in"
		//so cover_read_off should big(or equal) than "ring_read_off", small than "ring_write_off"
		if (cover_read_off >= ring_read_off && cover_read_off < ring_write_off)
		{
			real_can_read_len = ring_available_data - (cover_read_off - ring_read_off);
		}
		//ELSE if "cover_read_off" not in range above, that means the data you want to read is already covered.
	}
	else
	{
		//current ring read from "out --> end" AND "0 --> in" 
		do
		{
			if (cover_read_off > ring_write_off && cover_read_off < ring_read_off)
			{
				//cover_read_off shouldn't in this range. if in, that means the data you want to read is already covered.
				break;
			}
			if (cover_read_off >= ring_read_off)
			{
				//cover_read_off is in "out->end"
				real_can_read_len = ring_available_data - (cover_read_off - ring_read_off);
			}
			else
			{
				//cover_read_off is in "0->in", so "0->cover_read_off" should discard.
				real_can_read_len = ring_write_off - cover_read_off;
			}
		} while (0);
	}
	ASSERT_ABORT(real_can_read_len <= (int)ring_available_data);

	if (target && real_can_read_len >= (int)request_read_len)
	{
		//perform discard
		uint32_t should_discard_len = ring_available_data - real_can_read_len;
		if (should_discard_len)
		{
			RingBuffer_discard(buf_handle->ring, should_discard_len);
		}
		uint32_t real_read_out_len = RingBuffer_read(buf_handle->ring, target, request_read_len);
		ASSERT_ABORT(real_read_out_len == request_read_len);
	}

	AUTOCOVER_UNLOCK(buf_handle);
	return real_can_read_len;
}

int auto_cover_buf_available_read(const auto_cover_buf_handle buf_handle, uint32_t read_pos)
{
	return auto_cover_buf_internal_read(buf_handle, read_pos, NULL, 0);
}

int auto_cover_buf_read(const auto_cover_buf_handle buf_handle, uint32_t read_pos, char* target, uint32_t req_read_len)
{
	ASSERT_RET_VALUE(target, AUTO_COVER_BUF_ERR_INVALID_HANDLE);
	ASSERT_RET_VALUE(req_read_len, AUTO_COVER_BUF_ERR_INVALID_PARAM);
	int available_read_data = auto_cover_buf_internal_read(buf_handle, read_pos, target, req_read_len);
	if (available_read_data < 0)
	{
		//error: data is covered.
		return available_read_data;
	}
	int real_read_len;
	if (available_read_data < (int)req_read_len)
	{
		//error: not enough data, we NOT perform read operation.
		real_read_len = AUTO_COVER_BUF_ERROR_DATA_NOT_ENOUGH;
	}
	else
	{
		//we have enough data to read, and already read it out.
		real_read_len = req_read_len;
	}
	return real_read_len;
}

int auto_cover_buf_write(const auto_cover_buf_handle buf_handle, char* source, uint32_t write_len)
{
	ASSERT_RET_VALUE(buf_handle, AUTO_COVER_BUF_ERR_INVALID_HANDLE);
	ASSERT_RET_VALUE(source, AUTO_COVER_BUF_ERR_INVALID_HANDLE);
	AUTOCOVER_LOCK(buf_handle);

	uint32_t ring_available_write = RingBuffer_available_write(buf_handle->ring);
	if (write_len > ring_available_write)
	{
		RingBuffer_discard(buf_handle->ring, write_len - ring_available_write);
	}
	uint32_t real_write_len = RingBuffer_write(buf_handle->ring, source, write_len);
	ASSERT_ABORT(real_write_len == write_len);

	AUTOCOVER_UNLOCK(buf_handle);
	return (int)real_write_len;
}

void auto_cover_buf_destroy(auto_cover_buf_handle* buf_handle_p)
{
	if (buf_handle_p ==  NULL || *buf_handle_p == NULL)
	{
		return;
	}
	auto_cover_buf_handle buf_handle = *buf_handle_p;
	RingBuffer_destroy(&buf_handle->ring);
	free(buf_handle);
	*buf_handle_p = NULL;
}