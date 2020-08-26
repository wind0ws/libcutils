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
#include <stdlib.h>
#include <string.h>
#include "mem/allocator.h"
#include "mem/allocation_tracker.h"

 //we define alloc function to lcu_alloc, 
 //so here we should undef it to avoid Recursive call. 
#ifdef _USE_LCU_MEM_CHECK 
#undef malloc
#undef free
#undef calloc
#undef realloc
#undef strdup
#undef strndup
#endif // _USE_LCU_MEM_CHECK

static const allocator_id_t alloc_allocator_id = 99;

char* lcu_strdup_trace(const char* str, const char* file_path, const char* func_name, int file_line)
{
	size_t size = strlen(str) + 1;  // + 1 for the null terminator
	size_t real_size = allocation_tracker_resize_for_canary(size);
	void* ptr = malloc(real_size);
	if (!ptr)
	{
		return NULL;
	}
	char* new_string = allocation_tracker_notify_alloc(alloc_allocator_id,
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
	size_t real_size = allocation_tracker_resize_for_canary(size + 1);
	void* ptr = malloc(real_size);
	if (!ptr)
	{
		return NULL;
	}
	char* new_string = allocation_tracker_notify_alloc(alloc_allocator_id,
		ptr, size + 1, file_path, func_name, file_line);
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
	size_t real_size = allocation_tracker_resize_for_canary(size);
	void* ptr = malloc(real_size);
	if (!ptr)
	{
		return NULL;
	}
	return allocation_tracker_notify_alloc(alloc_allocator_id, ptr, size, file_path, func_name, file_line);
}

void* lcu_malloc(size_t size)
{
	return lcu_malloc_trace(size, NULL, NULL, -1);
}

void* lcu_calloc_trace(size_t item_count, size_t item_size, const char* file_path, const char* func_name, int file_line)
{
	size_t request_size = item_count * item_size;
	size_t real_size = allocation_tracker_resize_for_canary(request_size);
	void* ptr = calloc(1, real_size);
	if (!ptr)
	{
		return NULL;
	}
	return allocation_tracker_notify_alloc(alloc_allocator_id, ptr, request_size, file_path, func_name, file_line);
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
	if (size == 0)
	{
		//if size == 0, free the ptr, return NULL
		if (ptr)
		{
			lcu_free(ptr);
		}
		return NULL;
	}

	if (ptr == NULL)
	{
		/* a little trick: give more memory than you need, for reduce realloc times */
		return lcu_malloc_trace(size + 2048, file_path, func_name, file_line);
	}

	size_t cur_ptr_size = allocation_tracker_ptr_size(alloc_allocator_id, ptr);
	if (cur_ptr_size && size <= cur_ptr_size)
	{
		//current size if enough, no need alloc new memory.
		return ptr;
	}

	/* a little trick: give more memory than you need, for reduce realloc times */
	void* new_ptr = lcu_malloc_trace(size + 2048, file_path, func_name, file_line);
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
	void* real_ptr = allocation_tracker_notify_free(alloc_allocator_id, ptr);
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