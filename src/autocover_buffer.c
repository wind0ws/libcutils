#include "autocover_buffer.h"
#include "ringbuffer.h"
#include "common_macro.h"
#include <malloc.h>
#include <string.h>

struct __auto_cover_buf
{
	ring_buf ring;
	uint32_t ring_buffer_size;
	auto_cover_buf_lock_fn lockfn;
	auto_cover_buf_unlock_fn unlockfn;
	void * lock_unlock_fn_user_data;
};

#define AUTOCOVER_LOCK(cover_buf_p) 	if (cover_buf_p->lockfn)\
{\
	cover_buf_p->lockfn(cover_buf_p->lock_unlock_fn_user_data);\
};
#define AUTOCOVER_UNLOCK(cover_buf_p) 	if (cover_buf_p->unlockfn)\
{\
	cover_buf_p->unlockfn(cover_buf_p->lock_unlock_fn_user_data);\
};


auto_cover_buf auto_cover_buf_create(uint32_t capacity_size, auto_cover_buf_lock_fn lockfn, auto_cover_buf_lock_fn unlockfn, void* lock_unlock_fn_user_data)
{
	auto_cover_buf cover_buf_p = (auto_cover_buf)malloc(sizeof(struct __auto_cover_buf));
	ASSERT_RET_VALUE(cover_buf_p, NULL);
	cover_buf_p->lock_unlock_fn_user_data = lock_unlock_fn_user_data;
	cover_buf_p->lockfn = lockfn;
	cover_buf_p->unlockfn = unlockfn;
	cover_buf_p->ring = RingBuffer_create(capacity_size);
	ASSERT_ABORT(cover_buf_p->ring);
	cover_buf_p->ring_buffer_size = RingBuffer_real_capacity(cover_buf_p->ring);
	return cover_buf_p;
}

/**
 * read cover buf from read_pos
 * @return if return value below 0, that means the data you want to read start from read_pos is already covered.
 */
static int auto_cover_buf_internal_read(const auto_cover_buf cover_buf_p, uint32_t read_pos, char* target, uint32_t request_read_len)
{
	ASSERT_RET_VALUE(cover_buf_p, -1);
	AUTOCOVER_LOCK(cover_buf_p);

	int real_can_read_len = -2;
	uint32_t cover_read_off = read_pos & (cover_buf_p->ring_buffer_size - 1);
	uint32_t ring_available_data = RingBuffer_available_read(cover_buf_p->ring);
	uint32_t ring_read_off = RingBuffer_current_read_position(cover_buf_p->ring) & (cover_buf_p->ring_buffer_size - 1);
	uint32_t ring_write_off = RingBuffer_current_write_position(cover_buf_p->ring) & (cover_buf_p->ring_buffer_size - 1);
	if (ring_write_off - ring_read_off == ring_available_data)
	{
		//current ring read from out --> in
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
				//cover_read_off is in "0->in", so 0->cover_read_off should discard.
				real_can_read_len = ring_write_off - cover_read_off;
			}
		} while (0);
	}

	if (target && real_can_read_len >= (int)request_read_len)
	{
		//perform discard
		ASSERT_ABORT(ring_available_data >= real_can_read_len);
		uint32_t should_discard_len = ring_available_data - real_can_read_len;
		if (should_discard_len)
		{
			RingBuffer_discard(cover_buf_p->ring, should_discard_len);
		}
		RingBuffer_read(cover_buf_p->ring, target, request_read_len);
	}

	AUTOCOVER_UNLOCK(cover_buf_p);
	return real_can_read_len;
}

int auto_cover_buf_available_read(const auto_cover_buf cover_buf_p, uint32_t read_pos)
{
	return auto_cover_buf_internal_read(cover_buf_p, read_pos, NULL, 0);
}

int auto_cover_buf_read(const auto_cover_buf cover_buf_p, uint32_t read_pos, char* target, uint32_t req_read_len)
{
	int available_read_data = auto_cover_buf_internal_read(cover_buf_p, read_pos, target, req_read_len);
	if (available_read_data < 0)
	{
		//data is covered.
		return available_read_data;
	}
	int real_read_len = available_read_data;
	if (available_read_data < req_read_len)
	{
		//if not enough, we not perform read operation.
		real_read_len = -3;
	}
	else
	{
		//we have enough data to read, and already read it.
		real_read_len = req_read_len;
	}
	return real_read_len;
}

int auto_cover_buf_write(const auto_cover_buf cover_buf_p, char* source, uint32_t write_len)
{
	ASSERT_RET_VALUE(cover_buf_p, -1);
	AUTOCOVER_LOCK(cover_buf_p);

	uint32_t ring_available_write = RingBuffer_available_write(cover_buf_p->ring);
	if (write_len > ring_available_write)
	{
		RingBuffer_discard(cover_buf_p->ring, write_len - ring_available_write);
	}
	uint32_t real_write_len = RingBuffer_write(cover_buf_p->ring, source, write_len);
	ASSERT_ABORT(real_write_len == write_len);

	AUTOCOVER_UNLOCK(cover_buf_p);
	return (int)real_write_len;
}

void auto_cover_buf_destroy(auto_cover_buf* cover_buf_pp)
{
	if (cover_buf_pp ==  NULL || *cover_buf_pp == NULL)
	{
		return;
	}
	auto_cover_buf cover_buf_p = *cover_buf_pp;
	RingBuffer_destroy(cover_buf_p->ring);
	free(cover_buf_p);
	*cover_buf_pp = NULL;
}