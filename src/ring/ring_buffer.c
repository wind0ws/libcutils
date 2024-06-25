#include "ring/ring_buffer.h"
#include "log/slog.h"
#include <malloc.h> /* for malloc/free */
#include <string.h> /* for memcpy      */

#define _RING_LOG_TAG         "RING_BUF"

#define RING_LOGV(fmt,...)    SLOGV(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGD(fmt,...)    SLOGD(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGI(fmt,...)    SLOGI(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGW(fmt,...)    SLOGW(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGE(fmt,...)    SLOGE(_RING_LOG_TAG, fmt, ##__VA_ARGS__)

#ifndef TAKE_MIN
/* take min value of a,b */
#define TAKE_MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif // TAKE_MIN

//config try read/write if no enough data or space
// if enable, maybe you are not full read/write data, be aware of this.
#define RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH (0)

/*
** the bitwise version :
** we apply (n - 1) mask to n, and then check that is equal to 0
** it will be true for all numbers that are power of 2.
** Lastly we make sure that n is superior to 0.
*/
static inline int pri_is_power_of_2(uint32_t n)
{
	return (n > 1 && !(n & (n - 1)));
}

static inline uint32_t pri_roundup_pow_of_2(uint32_t x)
{
	if (x == 0 || pri_is_power_of_2(x))
	{
		return x;
	}
	// counter divide two times.
	uint32_t counter = 0;
	while (x >>= 1)
	{
		counter++;
	}
	return (uint32_t)(2 << counter);
}

struct _ring_buffer_t
{
	int need_free_myself;  /* mark struct and buffer whether is allocated by ourself */
	uint32_t in;           /* position of writer  */
	uint32_t out;          /* position of reader  */
	uint32_t size;         /* ring buffer size    */
	char* buf;             /* ring buffer pointer, buf size must be power of 2 */
};

#define _RING_AVAILABLE_READ(ring_handle)  (ring_handle->in - ring_handle->out)
#define _RING_AVAILABLE_WRITE(ring_handle) (ring_handle->size - (_RING_AVAILABLE_READ(ring_handle)))

ring_buffer_handle ring_buffer_create_with_mem(__in void* buf, __in uint32_t buf_size)
{
	if (!buf)
	{
		return NULL;
	}
	const uint32_t ring_buffer_struct_size = (uint32_t)sizeof(struct _ring_buffer_t);
	if (buf_size < ring_buffer_struct_size + 4U)
	{
		RING_LOGE("invalid buf_size: %u", buf_size);
		return NULL;
	}
	uint32_t ring_buf_size = buf_size - ring_buffer_struct_size;
	if (!pri_is_power_of_2(ring_buf_size))
	{
		RING_LOGW("%s buf_size=%u is not power of 2", __func__, ring_buf_size);
		ring_buf_size = (pri_roundup_pow_of_2(ring_buf_size) >> 1);
		RING_LOGW("%s changed buf_size to %u ", __func__, ring_buf_size);
	}
	ring_buffer_handle ring_buffer_p = (ring_buffer_handle)buf;
	ring_buffer_p->need_free_myself = 0;
	ring_buffer_p->buf = (char *)buf + ring_buffer_struct_size;
	ring_buffer_p->size = ring_buf_size;
	ring_buffer_p->in = 0;
	ring_buffer_p->out = 0;
	memset(ring_buffer_p->buf, 0, ring_buffer_p->size);
	return ring_buffer_p;
}

ring_buffer_handle ring_buffer_create(__in uint32_t buf_size)
{
	if (buf_size < 2U)
	{
		RING_LOGE("invalid buf_size: %u", buf_size);
		return NULL;
	}
	if (!pri_is_power_of_2(buf_size))
	{
		RING_LOGW("%s buf_size=%u is not power of 2", __func__, buf_size);
		buf_size = pri_roundup_pow_of_2(buf_size);
		RING_LOGW("%s changed buf_size to %u ", __func__, buf_size);
	}

	const uint32_t expect_memory_size = sizeof(struct _ring_buffer_t) + buf_size;
	char *raw_memory = (char *)calloc(1, expect_memory_size);
	if (!raw_memory)
	{
		RING_LOGE("failed alloc %u size for RingBuffer", expect_memory_size);
		return NULL;
	}
	ring_buffer_handle ring_buffer_p = ring_buffer_create_with_mem(raw_memory, expect_memory_size);
	if (!ring_buffer_p)
	{
		free(raw_memory);
		return NULL;
	}
	ring_buffer_p->need_free_myself = 1;
	return ring_buffer_p;
}

void ring_buffer_destroy(__inout ring_buffer_handle* ring_handle_p)
{
	if (NULL == ring_handle_p || NULL == *ring_handle_p)
	{
		return;
	}
	ring_buffer_handle ring_handle = *ring_handle_p;
	ring_handle->in = 0;
	ring_handle->out = 0;
	ring_handle->size = 0;

	if (ring_handle->need_free_myself)
	{
		ring_handle->buf = NULL; // buf memory is in handle
		ring_handle->need_free_myself = 0;
		free(ring_handle);
	}
	*ring_handle_p = NULL;
}

extern inline bool ring_buffer_is_empty(__in ring_buffer_handle ring_handle)
{
	return 0 == _RING_AVAILABLE_READ(ring_handle); // ring_buffer_available_read(ring_handle) == 0;
}

extern inline bool ring_buffer_is_full(__in ring_buffer_handle ring_handle)
{
	return 0 == _RING_AVAILABLE_WRITE(ring_handle); // RingBuffer_available_write(ring_handle) == 0;
}

extern inline void ring_buffer_clear(__in ring_buffer_handle ring_handle)
{
	if (NULL == ring_handle)
	{
		return;
	}
	ring_handle->in = 0;
	ring_handle->out = 0;
}

uint32_t ring_buffer_current_read_position(__in ring_buffer_handle ring_handle)
{
	return ring_handle->out;
}

uint32_t ring_buffer_current_write_position(__in ring_buffer_handle ring_handle)
{
	return ring_handle->in;
}

uint32_t ring_buffer_real_capacity(__in ring_buffer_handle ring_handle) 
{
	return ring_handle->size;
}

extern inline uint32_t ring_buffer_available_read(__in ring_buffer_handle ring_handle)
{
	return _RING_AVAILABLE_READ(ring_handle); // ring_handle->in - ring_handle->out;
}

extern inline uint32_t ring_buffer_available_write(__in ring_buffer_handle ring_handle)
{
	return _RING_AVAILABLE_WRITE(ring_handle); // ring_handle->size - ring_buffer_available_read(ring_handle);
}

uint32_t ring_buffer_write(__in ring_buffer_handle ring_handle, __in const void* source, __in uint32_t size)
{
	if (NULL == ring_handle || NULL == source || 0 == size)
	{
		return 0;
	}
	uint32_t start = 0, first_part_len = 0, rest_len = 0;
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	uint32_t origin_size = size;
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	const uint32_t available_space = _RING_AVAILABLE_WRITE(ring_handle); //RingBuffer_available_write(ring_handle);
	if (0 == available_space)
	{
		return 0;
	}
	size = TAKE_MIN(size, available_space);
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	if (origin_size != size)
	{
		return 0;
	}
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	/* first put the data starting from fifo->in to buffer end */
	start = ring_handle->in & (ring_handle->size - 1U);
	const uint32_t first_part_max_len = ring_handle->size - start;
	first_part_len = TAKE_MIN(size, first_part_max_len);
	if (first_part_len)
	{
		memcpy(ring_handle->buf + start, source, first_part_len);
	}
	/* then put the rest_len (if any) at the beginning of the buffer */
	if ((rest_len = size - first_part_len) > 0)
	{
		memcpy(ring_handle->buf, (char*)source + first_part_len, rest_len);
	}
	ring_handle->in += size;
	return size;
}

/**
 * @brief skip read_offset and copy read_size memory from RingBuffer to target.
 * If current RingBuffer can read data size is little than (read_offset + read_size),
 * then only copy max readable data size to target.
 *
 * @param[in]  ring_handle handle of ring buffer
 * @param[in]  _read_offset skip read_offset
 * @param[out] _target the target pointer to write data
 * @param[in]  _read_size   the size you want to copy to target
 * @param[in]  _is_peek     indicate peek or real read(with move ringbuffer.read_offset)
 *
 * @return the real read data size of target. NOT ring buffer internal read size!
 */
static uint32_t pri_ring_buffer_read_internal(ring_buffer_handle ring_handle, 
	uint32_t _read_offset, void* _target, uint32_t _read_size, bool _is_peek)
{
	if (NULL == ring_handle || 0 == _read_size)
	{
		return 0;
	}
	char* target = (char *)_target;
	uint32_t left_read_offset = _read_offset;
	uint32_t left_read_size = _read_size;
	// total_read_size = skip_offset + read_size;
	uint32_t total_read_size = left_read_offset + left_read_size;
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	uint32_t origin_size = total_read_size;
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	const uint32_t available_read_size = _RING_AVAILABLE_READ(ring_handle); //ring_buffer_available_read(ring_handle);
	if (0 == available_read_size)
	{
		return 0;
	}
	total_read_size = TAKE_MIN(total_read_size, available_read_size);
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	if (origin_size != total_read_size)
	{
		return 0;
	}
#else
	// make sure total_read_size bigger than read_offset, otherwise we just forbid read.
	if (total_read_size <= (left_read_offset + 1U))
	{
		return 0;
	}
	// rearrange read size.
	_read_size = total_read_size - left_read_offset;
	left_read_size = _read_size;
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH

	if (target)
	{
		/* 1. first: get the data from fifo->out until the end of the buffer */
		const uint32_t start = (ring_handle->out & (ring_handle->size - 1U));
		const uint32_t first_part_max_len = ring_handle->size - start;
		const uint32_t first_part_len = TAKE_MIN(total_read_size, first_part_max_len);
		if (first_part_len)
		{
			// if first_part_len bigger(or equal) read_offset, just skip the offset, 
			// then get min data size of expected, and copy rest to target.
			if (first_part_len >= left_read_offset)
			{
				const uint32_t first_part_len_after_skip_offset = first_part_len - left_read_offset;
				if (first_part_len_after_skip_offset > 0)
				{
					const uint32_t first_part_real_write_to_target_len = TAKE_MIN(left_read_size, first_part_len_after_skip_offset);
					left_read_size -= first_part_real_write_to_target_len;
					memcpy(target, ring_handle->buf + start + left_read_offset, first_part_real_write_to_target_len);
					target += first_part_real_write_to_target_len;
				}
				left_read_offset = 0;
			}
			else
			{
				left_read_offset -= first_part_len;
			}
			//memcpy(target, ring_handle->buf + start, first_part_len);
			//target += first_part_len;
		}

		/* 2. second: then we get the rest_len (if any) from the beginning of the buffer */
		const uint32_t rest_len = total_read_size - first_part_len;
		if (rest_len)
		{
			if (left_read_size > 0)	// we are sure that we have enough data to read out.
			{
				memcpy(target, ring_handle->buf + left_read_offset, left_read_size);
				left_read_size = 0;
				left_read_offset = 0;
			}
			//memcpy(target, ring_handle->buf, rest_len);
		}
	}

	if (!_is_peek)
	{
		ring_handle->out += total_read_size;
	}
	
	// we make sure the _read_size can be full read.
	return _read_size;
	//return total_read_size;
}

uint32_t ring_buffer_read(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size)
{
	return NULL == target ? 0 : pri_ring_buffer_read_internal(ring_handle, 0, target, size, false);
}

uint32_t ring_buffer_read_with_offset(__in ring_buffer_handle ring_handle,
	__in uint32_t read_offset, __out void* target, __in uint32_t size)
{
	return NULL == target ? 0 : pri_ring_buffer_read_internal(ring_handle, read_offset, target, size, false);
}

uint32_t ring_buffer_peek(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size)
{
	return pri_ring_buffer_read_internal(ring_handle, 0, target, size, true);
}

uint32_t ring_buffer_peek_with_offset(__in ring_buffer_handle ring_handle,
	__in uint32_t peek_offset, __out void* target, __in uint32_t size)
{
	return pri_ring_buffer_read_internal(ring_handle, peek_offset, target, size, true);
}

uint32_t ring_buffer_discard(__in ring_buffer_handle ring_handle, __in uint32_t size)
{
	return pri_ring_buffer_read_internal(ring_handle, 0, NULL, size, false);
}
