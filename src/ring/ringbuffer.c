#include "ring/ringbuffer.h"
#include "log/slog.h"
#include <stdbool.h>
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
#define RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH (1)

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

static inline uint32_t pri_roundup_pow_of_two(uint32_t x)
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

struct __ring_buffer_t
{
	bool need_free_myself; /* mark struct and buffer whether is allocated by ourself */
	uint32_t in;           /* position of writer  */
	uint32_t out;          /* position of reader  */
	uint32_t size;         /* ring buffer size    */
	char* buf;             /* ring buffer pointer, buf size must be power of 2 */
};

#define _RING_AVAILABLE_READ(ring_handle)  (ring_handle->in - ring_handle->out)
#define _RING_AVAILABLE_WRITE(ring_handle) (ring_handle->size - (_RING_AVAILABLE_READ(ring_handle)))

ring_buffer_handle RingBuffer_create_with_mem(__in void* buf, __in uint32_t buf_size)
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
	if (!pri_is_power_of_2(ring_buf_size))
	{
		RING_LOGW("%s buf_size=%u is not power of 2", __func__, ring_buf_size);
		ring_buf_size = (pri_roundup_pow_of_two(ring_buf_size) >> 1);
		RING_LOGW("%s changed buf_size to %u ", __func__, ring_buf_size);
	}
	ring_buffer_handle ring_buffer_p = (ring_buffer_handle)buf;
	ring_buffer_p->need_free_myself = false;
	ring_buffer_p->buf = (char *)buf + ring_buffer_struct_size;
	ring_buffer_p->size = ring_buf_size;
	ring_buffer_p->in = 0;
	ring_buffer_p->out = 0;
	return ring_buffer_p;
}

ring_buffer_handle RingBuffer_create(__in uint32_t buf_size)
{
	if (buf_size < 2)
	{
		RING_LOGE("invalid buf_size: %u", buf_size);
		return NULL;
	}
	if (!pri_is_power_of_2(buf_size))
	{
		RING_LOGW("%s buf_size=%u is not power of 2", __func__, buf_size);
		buf_size = pri_roundup_pow_of_two(buf_size);
		RING_LOGW("%s changed buf_size to %u ", __func__, buf_size);
	}

	const uint32_t expect_memory_size = sizeof(struct __ring_buffer_t) + buf_size;
	char *raw_memory = (char *)calloc(1, expect_memory_size);
	if (!raw_memory)
	{
		RING_LOGE("failed alloc %u size for RingBuffer", expect_memory_size);
		return NULL;
	}
	ring_buffer_handle ring_buffer_p = RingBuffer_create_with_mem(raw_memory, expect_memory_size);
	if (!ring_buffer_p)
	{
		free(raw_memory);
		return NULL;
	}
	ring_buffer_p->need_free_myself = true;
	return ring_buffer_p;
}

void RingBuffer_destroy(__inout ring_buffer_handle* ring_handle_p)
{
	if (NULL == ring_handle_p || *ring_handle_p == NULL)
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
		ring_handle->need_free_myself = false;
		free(ring_handle);
	}
	*ring_handle_p = NULL;
}

extern inline bool RingBuffer_is_empty(__in ring_buffer_handle ring_handle)
{
	return 0 == _RING_AVAILABLE_READ(ring_handle); // RingBuffer_available_read(ring_handle) == 0;
}

extern inline bool RingBuffer_is_full(__in ring_buffer_handle ring_handle)
{
	return 0 == _RING_AVAILABLE_WRITE(ring_handle); // RingBuffer_available_write(ring_handle) == 0;
}

extern inline void RingBuffer_clear(__in ring_buffer_handle ring_handle)
{
	if (ring_handle == NULL)
	{
		return;
	}
	ring_handle->in = 0;
	ring_handle->out = 0;
}

uint32_t RingBuffer_current_read_position(__in ring_buffer_handle ring_handle)
{
	return ring_handle->out;
}

uint32_t RingBuffer_current_write_position(__in ring_buffer_handle ring_handle)
{
	return ring_handle->in;
}

uint32_t RingBuffer_real_capacity(__in ring_buffer_handle ring_handle) 
{
	return ring_handle->size;
}

extern inline uint32_t RingBuffer_available_read(__in ring_buffer_handle ring_handle)
{
	return _RING_AVAILABLE_READ(ring_handle); // ring_handle->in - ring_handle->out;
}

extern inline uint32_t RingBuffer_available_write(__in ring_buffer_handle ring_handle)
{
	return _RING_AVAILABLE_WRITE(ring_handle); // ring_handle->size - RingBuffer_available_read(ring_handle);
}

uint32_t RingBuffer_write(__in ring_buffer_handle ring_handle, __in const void* source, __in uint32_t size)
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
	start = ring_handle->in & (ring_handle->size - 1);
	uint32_t first_part_max_len = ring_handle->size - start;
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

static uint32_t RingBuffer_read_internal(ring_buffer_handle ring_handle, void* target, uint32_t size, bool is_peek)
{
	if (ring_handle == NULL || size == 0)
	{
		return 0;
	}
	uint32_t start = 0, first_part_len = 0, rest_len = 0;
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	uint32_t origin_size = size;
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	const uint32_t available_read_size = _RING_AVAILABLE_READ(ring_handle); //RingBuffer_available_read(ring_handle);
	if (0 == available_read_size)
	{
		return 0;
	}
	size = TAKE_MIN(size, available_read_size);
#if !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	if (origin_size != size)
	{
		return 0;
	}
#endif // !RINGBUF_CONFIG_TRY_RW_IF_NOT_ENOUGH
	if (target)
	{
		/* first get the data from fifo->out until the end of the buffer */
		start = ring_handle->out & (ring_handle->size - 1);
		uint32_t first_part_max_len = ring_handle->size - start;
		first_part_len = TAKE_MIN(size, first_part_max_len);
		if (first_part_len)
		{
			memcpy(target, ring_handle->buf + start, first_part_len);
		}
		/* then get the rest_len (if any) from the beginning of the buffer */
		rest_len = size - first_part_len;
		if (rest_len)
		{
			memcpy((char*)target + first_part_len, ring_handle->buf, rest_len);
		}
	}
	if (!is_peek)
	{
		ring_handle->out += size;
	}
	return size;
}

uint32_t RingBuffer_read(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size)
{
	return target == NULL ? 0 : RingBuffer_read_internal(ring_handle, target, size, false);
}

uint32_t RingBuffer_peek(__in ring_buffer_handle ring_handle, __out void* target, __in uint32_t size)
{
	return RingBuffer_read_internal(ring_handle, target, size, true);
}

uint32_t RingBuffer_discard(__in ring_buffer_handle ring_handle, __in uint32_t size)
{
	return RingBuffer_read_internal(ring_handle, NULL, size, false);
}
