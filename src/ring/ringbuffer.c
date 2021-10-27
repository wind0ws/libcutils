#include "ring/ringbuffer.h"
#include "log/simple_log.h"
#include <stdbool.h>
#include <malloc.h> /* for malloc/free */
#include <string.h> /* for memcpy      */

#define _RING_LOG_TAG         "RING_BUF"

#define RING_LOGV(fmt,...)    SIMPLE_LOGV(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGD(fmt,...)    SIMPLE_LOGD(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGI(fmt,...)    SIMPLE_LOGI(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGW(fmt,...)    SIMPLE_LOGW(_RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGE(fmt,...)    SIMPLE_LOGE(_RING_LOG_TAG, fmt, ##__VA_ARGS__)

#ifndef TAKE_MIN
/* take min value of a,b */
#define TAKE_MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif // TAKE_MIN

//config try read/write if no enough data or space
#define RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH (1)

/*
** the bitwise version :
** we apply (n - 1) mask to n, and then check that is equal to 0
** it will be true for all numbers that are power of 2.
** Lastly we make sure that n is superior to 0.
*/
static inline int is_power_of_2(uint32_t n)
{
	return (n > 1 && !(n & (n - 1)));
}

static inline uint32_t roundup_pow_of_two(uint32_t x)
{
	if (x == 0 || is_power_of_2(x))
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

struct __ring_buffer_t
{
	bool need_free_myself; /* mark struct and buffer whether is allocated by ourself */
	uint32_t in;           /* position of writer  */
	uint32_t out;          /* position of reader  */
	uint32_t size;         /* ring buffer size    */
	char* buf;             /* ring buffer pointer, buf size must be power of 2 */
};

ring_buf_handle RingBuffer_create_with_mem(__in char* buf, __in uint32_t buf_size)
{
	if (!buf)
	{
		return NULL;
	}
	uint32_t ring_buffer_struct_size = (uint32_t)sizeof(struct __ring_buffer_t);
	if (buf_size < ring_buffer_struct_size + 4)
	{
		RING_LOGE("invalid buf_size: %u", buf_size);
		return NULL;
	}
	uint32_t ring_buf_size = buf_size - ring_buffer_struct_size;
	if (!is_power_of_2(ring_buf_size))
	{
		RING_LOGW("RingBuffer_create_with_mem buf_size=%u is not power of 2", ring_buf_size);
		ring_buf_size = roundup_pow_of_two(ring_buf_size) >> 1;
		RING_LOGW("RingBuffer_create_with_mem changed buf_size to %u ", ring_buf_size);
	}
	ring_buf_handle ring_buffer_p = (ring_buf_handle)buf;
	ring_buffer_p->need_free_myself = false;
	ring_buffer_p->buf = buf + ring_buffer_struct_size;
	ring_buffer_p->size = ring_buf_size;
	ring_buffer_p->in = 0;
	ring_buffer_p->out = 0;
	return ring_buffer_p;
}

ring_buf_handle RingBuffer_create(__in uint32_t buf_size)
{
	if (buf_size < 2)
	{
		RING_LOGE("invalid buf_size: %u", buf_size);
		return NULL;
	}
	if (!is_power_of_2(buf_size))
	{
		RING_LOGW("RingBuffer_create buf_size=%u is not power of 2", buf_size);
		buf_size = roundup_pow_of_two(buf_size);
		RING_LOGW("RingBuffer_create changed buf_size to %u ", buf_size);
	}

	const uint32_t expect_memory_size = sizeof(struct __ring_buffer_t) + buf_size;
	char *raw_memory = (char *)calloc(1, expect_memory_size);
	if (raw_memory == NULL)
	{
		RING_LOGE("failed alloc %u size for RingBuffer", expect_memory_size);
		return NULL;
	}
	ring_buf_handle ring_buffer_p = RingBuffer_create_with_mem(raw_memory, expect_memory_size);
	if (!ring_buffer_p)
	{
		free(raw_memory);
		return NULL;
	}
	ring_buffer_p->need_free_myself = true;
	return ring_buffer_p;
}

void RingBuffer_destroy(__in ring_buf_handle* ring_buf_pp)
{
	if (ring_buf_pp == NULL || *ring_buf_pp == NULL)
	{
		return;
	}
	ring_buf_handle ring_buffer_p = *ring_buf_pp;
	ring_buffer_p->in = 0;
	ring_buffer_p->out = 0;
	ring_buffer_p->size = 0;

	if (ring_buffer_p->need_free_myself)
	{
		ring_buffer_p->buf = NULL; // buf memory is in handle
		ring_buffer_p->need_free_myself = false;
		free(ring_buffer_p);
	}
	*ring_buf_pp = NULL;
}

extern inline bool RingBuffer_is_empty(__in const ring_buf_handle ring_buf_p)
{
	return RingBuffer_available_read(ring_buf_p) == 0;
}

extern inline bool RingBuffer_is_full(__in const ring_buf_handle ring_buf_p)
{
	return RingBuffer_available_write(ring_buf_p) == 0;
}

extern inline void RingBuffer_clear(__in const ring_buf_handle ring_buffer_p)
{
	if (ring_buffer_p == NULL)
	{
		return;
	}
	ring_buffer_p->in = 0;
	ring_buffer_p->out = 0;
}

uint32_t RingBuffer_current_read_position(__in const ring_buf_handle ring_buf_p)
{
	return ring_buf_p->out;
}

uint32_t RingBuffer_current_write_position(__in const ring_buf_handle ring_buf_p)
{
	return ring_buf_p->in;
}

uint32_t RingBuffer_real_capacity(__in const ring_buf_handle ring_buf_p) 
{
	return ring_buf_p->size;
}

extern inline uint32_t RingBuffer_available_read(__in const ring_buf_handle ring_buf_p)
{
	return ring_buf_p->in - ring_buf_p->out;
}

extern inline uint32_t RingBuffer_available_write(__in const ring_buf_handle ring_buf_p)
{
	return ring_buf_p->size - RingBuffer_available_read(ring_buf_p);
}

uint32_t RingBuffer_write(__in const ring_buf_handle ring_buf_p, __in const void* source, __in uint32_t size)
{
	if (ring_buf_p == NULL || source == NULL || size == 0)
	{
		return 0;
	}
	uint32_t start = 0, first_part_len = 0, rest_len = 0;
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	uint32_t origin_size = size;
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	uint32_t available_space = RingBuffer_available_write(ring_buf_p);
	if (available_space == 0)
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
	start = ring_buf_p->in & (ring_buf_p->size - 1);
	uint32_t first_part_max_len = ring_buf_p->size - start;
	first_part_len = TAKE_MIN(size, first_part_max_len);
	if (first_part_len)
	{
		memcpy(ring_buf_p->buf + start, source, first_part_len);
	}
	/* then put the rest_len (if any) at the beginning of the buffer */
	if ((rest_len = size - first_part_len) > 0)
	{
		memcpy(ring_buf_p->buf, (char*)source + first_part_len, rest_len);
	}
	ring_buf_p->in += size;
	return size;
}

static uint32_t RingBuffer_read_internal(ring_buf_handle ring_buf_p, void* target, uint32_t size, bool is_peek)
{
	if (ring_buf_p == NULL || size == 0)
	{
		return 0;
	}
	uint32_t start = 0, first_part_len = 0, rest_len = 0;
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	uint32_t origin_size = size;
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	uint32_t available_data = RingBuffer_available_read(ring_buf_p);
	if (available_data == 0)
	{
		return 0;
	}
	size = TAKE_MIN(size, available_data);
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	if (origin_size != size)
	{
		return 0;
	}
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	if (target)
	{
		/* first get the data from fifo->out until the end of the buffer */
		start = ring_buf_p->out & (ring_buf_p->size - 1);
		uint32_t first_part_max_len = ring_buf_p->size - start;
		first_part_len = TAKE_MIN(size, first_part_max_len);
		if (first_part_len)
		{
			memcpy(target, ring_buf_p->buf + start, first_part_len);
		}
		/* then get the rest_len (if any) from the beginning of the buffer */
		rest_len = size - first_part_len;
		if (rest_len)
		{
			memcpy((char*)target + first_part_len, ring_buf_p->buf, rest_len);
		}
	}
	if (!is_peek)
	{
		ring_buf_p->out += size;
	}
	return size;
}

uint32_t RingBuffer_read(__in ring_buf_handle ring_buf_p, __out void* target, uint32_t size)
{
	return target == NULL ? 0 : RingBuffer_read_internal(ring_buf_p, target, size, false);
}

uint32_t RingBuffer_peek(__in const ring_buf_handle ring_buf_p, __out void* target, uint32_t size)
{
	return RingBuffer_read_internal(ring_buf_p, target, size, true);
}

uint32_t RingBuffer_discard(__in const ring_buf_handle ring_buf_p, uint32_t size)
{
	return RingBuffer_read_internal(ring_buf_p, NULL, size, false);
}
