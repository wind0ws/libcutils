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
 * reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/allocator.h
 ******************************************************************************/

 /**
  * here I'm not suggest you use this header directly,
  * i recommend you use mem_debug.h
  */

#pragma once
#ifndef __LCU_ALLOCATOR_H
#define __LCU_ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef void* (*alloc_fn)(size_t size);
	typedef void (*free_fn)(void* ptr);
	typedef struct
	{
		alloc_fn alloc;
		free_fn  free;
	} allocator_t;

	// allocator_t abstractions for the lcu_*alloc and lcu_free functions
	extern const allocator_t allocator_malloc;
	extern const allocator_t allocator_calloc;

	char* lcu_strdup_trace(const char* str, const char* file_path, const char* func_name, int file_line);
	char* lcu_strdup(const char* str);

	char* lcu_strndup_trace(const char* str, size_t len, const char* file_path, const char* func_name, int file_line);
	char* lcu_strndup(const char* str, size_t len);

	void* lcu_malloc_trace(size_t size, const char* file_path, const char* func_name, int file_line);
	void* lcu_malloc(size_t size);

	void* lcu_calloc_trace(size_t item_count, size_t item_size, const char* file_path, const char* func_name, int file_line);
	void* lcu_calloc(size_t item_count, size_t item_size);

	/**
	 * realloc.
	 * Note: here have a performance issue.
	 * if pointer's memory is not enough that you requested,
	 * it will free it first, then alloc new memory, and memcpy old pointer's memory to new pointer.
	 * so, if you realloc a lot, maybe memcpy will perform a lot.
	 */
	void* lcu_realloc_trace(void* ptr, size_t size, const char* file_path, const char* func_name, int file_line);
	void* lcu_realloc(void* ptr, size_t size);

	void lcu_free(void* ptr);

	// Free a buffer that was previously allocated with function |lcu_malloc|
	// or |lcu_calloc| and reset the pointer to that buffer to NULL.
	// |p_ptr| is a pointer to the buffer pointer to be reset.
	// |p_ptr| cannot be NULL.
	void lcu_free_and_reset(void** p_ptr);

#ifdef __cplusplus
}
#endif

#endif // !__LCU_ALLOCATOR_H
