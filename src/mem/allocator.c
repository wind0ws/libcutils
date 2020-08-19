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

#include "lcu_stdafx.h"
#include <stdlib.h>
#include <string.h>
#include "mem/allocator.h"
#include "mem/allocation_tracker.h"
#include "common_macro.h"

static const allocator_id_t alloc_allocator_id = 42;

char* lcu_strdup(const char* str)
{
	size_t size = strlen(str) + 1;  // + 1 for the null terminator
	size_t real_size = allocation_tracker_resize_for_canary(size);
	void* ptr = malloc(real_size);
	ASSERT(ptr != NULL);
	char* new_string = allocation_tracker_notify_alloc(
		alloc_allocator_id,
		ptr,
		size);
	if (!new_string)
	{
		return NULL;
	}
	memcpy(new_string, str, size);
	return new_string;
}

char* lcu_strndup(const char* str, size_t len)
{
	size_t size = strlen(str);
	if (len < size)
	{
		size = len;
	}
	size_t real_size = allocation_tracker_resize_for_canary(size + 1);
	void* ptr = malloc(real_size);
	ASSERT(ptr);
	char* new_string = allocation_tracker_notify_alloc(
		alloc_allocator_id,
		ptr,
		size + 1);
	if (!new_string)
	{
		return NULL;
	}
	memcpy(new_string, str, size);
	new_string[size] = '\0';
	return new_string;
}

void* lcu_malloc(size_t size)
{
	size_t real_size = allocation_tracker_resize_for_canary(size);
	void* ptr = malloc(real_size);
	ASSERT(ptr != NULL);
	return allocation_tracker_notify_alloc(alloc_allocator_id, ptr, size);
}

void* lcu_calloc(size_t item_count, size_t item_size)
{
	size_t request_size = item_count * item_size;
	size_t real_size = allocation_tracker_resize_for_canary(request_size);
	void* ptr = calloc(1, real_size);
	ASSERT(ptr != NULL);
	return allocation_tracker_notify_alloc(alloc_allocator_id, ptr, request_size);
}

//Hidden method for allocator_t.
void* lcu_calloc1(size_t size)
{
	return lcu_calloc(1, size);
}

void* lcu_realloc(void* ptr, size_t size)
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
		return lcu_malloc(size + 2048); /* a little trick: give more memory than you need, for reduce realloc times */
	}

	size_t cur_ptr_size = allocation_tracker_ptr_size(alloc_allocator_id, ptr);
	if (cur_ptr_size && size <= cur_ptr_size)
	{
		//current size if enough, no need alloc new memory.
		return ptr;
	}

	void* new_ptr = lcu_malloc(size + 2048); /* a little trick: give more memory than you need, for reduce realloc times */
	if (!new_ptr)
	{
		return NULL;
	}
	memcpy(new_ptr, ptr, cur_ptr_size);
	lcu_free(ptr);
	return new_ptr;
}

void lcu_free(void* ptr)
{
	void* to_free = allocation_tracker_notify_free(alloc_allocator_id, ptr);
	if (to_free)
	{
		free(to_free);
	}
}

void lcu_free_and_reset(void** p_ptr)
{
	if (NULL == p_ptr || NULL  == *p_ptr)
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