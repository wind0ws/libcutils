/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/src/allocator.c
 ******************************************************************************/

#include "mem/mem_debug.h"
#include "common_macro.h"   /* L-1: for ASSERT (realloc tracker-uninit guard) */
#include <stdlib.h>
#include <string.h>
#include "mem/allocator.h"
#include "mem/allocation_tracker.h"

// we define alloc function to lcu_alloc, 
// so here we should undef it to avoid Recursive call. 
#ifdef _USE_LCU_MEM_CHECK 
#undef malloc
#undef free
#undef calloc
#undef realloc
#undef strdup
#undef strndup
#endif // _USE_LCU_MEM_CHECK

#define ALLOCTOR_ID  99

// Raw allocation functions: route straight to libc, bypass allocation tracking.
// After the #undef block above, malloc/calloc/free are always the libc symbols
// (whether or not _USE_LCU_MEM_CHECK is defined), so these never re-enter the
// tracker. Used by allocation_tracker's internal hashmap to break recursion.
static inline void* raw_malloc(size_t size)
{
	return malloc(size);
}

static inline void* raw_calloc(size_t size)
{
	return calloc(1, size);
}

static inline void raw_free(void* ptr)
{
	if (ptr)
	{
		free(ptr);
	}
}

static inline void* raw_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}

/* Public raw allocate/free: plain libc malloc/free, untracked. See allocator.h.
 * Defined after the #undef block above, so malloc/free are the libc symbols. */
void* lcu_malloc_raw(size_t size)
{
	return malloc(size);
}

void lcu_free_raw(void* ptr)
{
	if (ptr)
	{
		free(ptr);
	}
}

char* lcu_strdup_trace(const char* str, const char* file_path, const char* func_name, int file_line)
{
	size_t size = strlen(str) + 1U;  // + 1 for the null terminator
	size_t real_size = allocation_tracker_resize_for_canary(size);
	void* ptr = malloc(real_size);
	if (!ptr)
	{
		return NULL;
	}
	char* new_string = allocation_tracker_notify_alloc(ALLOCTOR_ID,
		ptr, size, file_path, func_name, file_line);
	if (!new_string)
	{
		return NULL;
	}
	memcpy(new_string, str, size);
	return new_string;
}

char* lcu_strdup(const char* str)
{
	return lcu_strdup_trace(str, NULL, NULL, -1);
}

char* lcu_strndup_trace(const char* str, size_t len, const char* file_path, const char* func_name, int file_line)
{
	size_t size = strlen(str);
	if (len < size)
	{
		size = len;
	}
	const size_t real_size = allocation_tracker_resize_for_canary(size + 1U);
	void* ptr = malloc(real_size);
	if (!ptr)
	{
		return NULL;
	}
	char* new_string = allocation_tracker_notify_alloc(ALLOCTOR_ID,
		ptr, size + 1U, file_path, func_name, file_line);
	if (!new_string)
	{
		return NULL;
	}
	memcpy(new_string, str, size);
	new_string[size] = '\0';
	return new_string;
}

char* lcu_strndup(const char* str, size_t len)
{
	return lcu_strndup_trace(str, len, NULL, NULL, -1);
}

void* lcu_malloc_trace(size_t size, const char* file_path, const char* func_name, int file_line)
{
	const size_t real_size = allocation_tracker_resize_for_canary(size);
	/* C-1 修复: 检查溢出。real_size == 0 且 size != 0 说明加 canary 溢出 */
	if (0 == real_size && size != 0)
	{
		return NULL;
	}
	void* ptr = malloc(real_size);
	if (!ptr)
	{
		return NULL;
	}
	return allocation_tracker_notify_alloc(ALLOCTOR_ID, ptr, size, file_path, func_name, file_line);
}

void* lcu_malloc(size_t size)
{
	return lcu_malloc_trace(size, NULL, NULL, -1);
}

void* lcu_calloc_trace(size_t item_count, size_t item_size, const char* file_path, const char* func_name, int file_line)
{
	/* Check for multiplication overflow */
	if (item_count != 0 && item_size > SIZE_MAX / item_count)
	{
		return NULL; /* Overflow would occur */
	}
	const size_t request_size = item_count * item_size;
	const size_t real_size = allocation_tracker_resize_for_canary(request_size);
	/* C-1 修复: 检查溢出 */
	if (0 == real_size && request_size != 0)
	{
		return NULL;
	}
	void* ptr = calloc(1, real_size);
	if (!ptr)
	{
		return NULL;
	}
	return allocation_tracker_notify_alloc(ALLOCTOR_ID, ptr, request_size, file_path, func_name, file_line);
}

void* lcu_calloc(size_t item_count, size_t item_size)
{
	return lcu_calloc_trace(item_count, item_size, NULL, NULL, -1);
}

//Hidden method for allocator_t.
void* lcu_calloc1(size_t size)
{
	return lcu_calloc(1, size);
}

void* lcu_realloc_trace(void* ptr, size_t size, const char* file_path, const char* func_name, int file_line)
{
	if (0 == size)
	{
		//if (0 == size), free the ptr, return NULL.
		if (ptr)
		{
			lcu_free(ptr);
		}
		return NULL;
	}

	/* Fix #1: when the tracker is inactive, every pointer is a plain libc
	 * pointer — delegate straight to libc realloc. This is also the correct
	 * behaviour for ptr==NULL (libc realloc(NULL,size) == malloc(size)) and it
	 * retires the old L-1 bug where a 0-byte memcpy silently dropped the data. */
	size_t cur_ptr_size = 0;
	if (!allocation_tracker_try_ptr_size(ALLOCTOR_ID, ptr, &cur_ptr_size))
	{
		/* Either the tracker is off, or |ptr| is an untracked raw/cross-boundary
		 * buffer (e.g. from file_util_read_all / strreplace / ini_parser_dump).
		 * Symmetric with lcu_free's raw fallback: realloc it via libc. The
		 * returned pointer stays untracked (no canary/leak coverage), exactly
		 * like the buffer it grew from. */
		return raw_realloc(ptr, size);
	}

	/* From here on the tracker is active AND |ptr| is a tracked block. */
	if (size <= cur_ptr_size)
	{
		//current size is enough, no need alloc new memory.
		return ptr;
	}

	/* Fix #4: allocate EXACTLY |size| (no +N over-allocation). The previous
	 * over-allocation pushed the tail canary |N| bytes past the user-visible
	 * end, blinding the corruption checker to [size, size+N) overruns. Correct
	 * detection outweighs realloc-churn amortization in a debug-only build. */
	void* new_ptr = lcu_malloc_trace(size, file_path, func_name, file_line);
	if (!new_ptr)
	{
		return NULL;
	}
	memcpy(new_ptr, ptr, cur_ptr_size);
	lcu_free(ptr);
	return new_ptr;
}

void* lcu_realloc(void* ptr, size_t size)
{
	return lcu_realloc_trace(ptr, size, NULL, NULL, -1);
}

void lcu_free(void* ptr)
{
	if (!ptr)
	{
		return;
	}
	void* real_ptr = allocation_tracker_notify_free(ALLOCTOR_ID, ptr);
	if (real_ptr)
	{
		free(real_ptr);
	}
}

void lcu_free_and_reset(void** p_ptr)
{
	if (NULL == p_ptr || NULL == *p_ptr)
	{
		return;
	}
	lcu_free(*p_ptr);
	*p_ptr = NULL;
}

const allocator_t allocator_calloc =
{
  lcu_calloc1,
  lcu_free
};

const allocator_t allocator_malloc =
{
  lcu_malloc,
  lcu_free
};

const allocator_t allocator_calloc_raw =
{
  raw_calloc,
  raw_free
};

const allocator_t allocator_malloc_raw =
{
  raw_malloc,
  raw_free
};
