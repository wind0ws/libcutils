#include "ring/ring_buf.h"
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include "log/slog.h"

#define RING_BUF_TAKE_MIN(a, b) ((a) > (b) ? (b) : (a))

#define __RING_LOG_TAG        "R_BUF"

#define RING_LOGV(fmt,...)    SLOGV(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGD(fmt,...)    SLOGD(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGI(fmt,...)    SLOGI(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGW(fmt,...)    SLOGW(__RING_LOG_TAG, fmt, ##__VA_ARGS__)
#define RING_LOGE(fmt,...)    SLOGE(__RING_LOG_TAG, fmt, ##__VA_ARGS__)

struct __ring_buf_t 
{
	bool need_free_myself;
	char* pbuf;
	size_t size;
	size_t offset_read;
	size_t offset_write;
};

ring_handle ring_buf_create_with_mem(void* pbuf, const size_t buf_size)
{
	if (!pbuf)
	{
		return NULL;
	}
	const size_t ring_struct_size = sizeof(struct __ring_buf_t);
	if (buf_size < ring_struct_size + 3)
	{
		RING_LOGE("buf_size(%zu) is too small", buf_size);
		return NULL;
	}
	ring_handle handle = (ring_handle)pbuf;
	handle->need_free_myself = false;
	handle->size = buf_size - ring_struct_size;
	handle->pbuf = (char*)pbuf + ring_struct_size;
	handle->offset_read = handle->offset_write = 0;
	return handle;
}

ring_handle ring_buf_create(const size_t size) 
{
	if (size < 3)
	{
		RING_LOGE("buf size too small. %zu", size);
		return NULL;
	}
	const size_t expect_mem_size = size + sizeof(struct __ring_buf_t);
	char* pbuf = (char*)malloc(expect_mem_size);
	if (!pbuf) 
	{
		RING_LOGE("failed alloc %zu size for ring_buf", expect_mem_size);
		return NULL;
	}
	ring_handle handle = ring_buf_create_with_mem(pbuf, expect_mem_size);
	if (!handle) 
	{
		RING_LOGE("failed alloc ring_handle");
		free(pbuf);
		return NULL;
	}
	handle->need_free_myself = true;
	return handle;
}

extern inline size_t ring_buf_available_read(ring_handle handle) 
{
	return (handle->size + handle->offset_write - handle->offset_read) % (handle->size);
}

extern inline size_t ring_buf_available_write(ring_handle handle) 
{
	return handle->size - ring_buf_available_read(handle) - 1;
}

static size_t internal_ring_buf_read(ring_handle handle, bool is_peek, void* target, size_t len) 
{
	if (len == 0 || len > ring_buf_available_read(handle)) 
	{
		return 0;
	}

	//first part: from read offset to end.
	size_t first_part_max_len = handle->size - handle->offset_read;
	size_t first_part_len = RING_BUF_TAKE_MIN(len, first_part_max_len);
	if (target)
	{
		memcpy(target, handle->pbuf + handle->offset_read, first_part_len);
	}

	//second part: from begin
	size_t second_part_len = len - first_part_len;
	if (second_part_len) 
	{
		if (target)
		{
			memcpy((char*)target + first_part_len, handle->pbuf, second_part_len);
		}
		if (!is_peek)
		{
			handle->offset_read = second_part_len;
		}
	}
	else 
	{
		if (!is_peek)
		{
			size_t new_offset_read = handle->offset_read + first_part_len;
			if (new_offset_read == handle->size)
			{
				new_offset_read = 0;
			}
			handle->offset_read = new_offset_read;
		}
	}
	return len;
}

size_t ring_buf_read(ring_handle handle, void* target, size_t len) 
{
	return internal_ring_buf_read(handle, false, target, len);
}

size_t ring_buf_peek(ring_handle handle, void* target, size_t len) 
{
	return internal_ring_buf_read(handle, true, target, len);
}

size_t ring_buf_discard(ring_handle handle, size_t len) 
{
	return internal_ring_buf_read(handle, false, NULL, len);
}

size_t ring_buf_write(ring_handle handle, void* source, size_t len) 
{
	if (len == 0 || len > ring_buf_available_write(handle))
	{
		return 0;
	}

	//first part: from write offset to end.
	size_t first_part_max_len = handle->size - handle->offset_write;
	size_t first_part_len = RING_BUF_TAKE_MIN(len, first_part_max_len);
	if (first_part_len) 
	{
		memcpy(handle->pbuf + handle->offset_write, source, first_part_len);
	}

	//second part: from begin
	size_t second_part_len = len - first_part_len;
	if (0 == second_part_len) 
	{
		size_t new_offset_write = handle->offset_write + first_part_len;
		if (new_offset_write == handle->size)
		{
			new_offset_write = 0;
		}
		handle->offset_write = new_offset_write;
	}
	else 
	{
		memcpy(handle->pbuf, (char*)source + first_part_len, second_part_len);
		handle->offset_write = second_part_len;
	}
	return len;
}

void ring_buf_clear(ring_handle handle) 
{
	handle->offset_read = handle->offset_write = 0;
}

void ring_buf_destroy(ring_handle* handle_p) 
{
	if (!handle_p) 
	{
		return;
	}
	ring_handle handle = *handle_p;
	if (!handle) 
	{
		return;
	}
	if (handle->need_free_myself) 
	{
		handle->need_free_myself = false;
		handle->pbuf = NULL;// buf memory is in handle
		free(handle);
	}
	*handle_p = NULL;
}
