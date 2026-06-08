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
#ifndef _LCU_ALLOCATOR_H
#define _LCU_ALLOCATOR_H

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

	/**
	 * @brief Raw allocators that bypass allocation tracking.
	 *
	 * These allocators route directly to libc malloc/calloc, bypassing the
	 * allocation_tracker system entirely.
	 *
	 * @warning **CRITICAL RECURSION BREAKER**: These exist ONLY to break the
	 * recursion loop that occurs when allocation_tracker's internal storage
	 * (hashmap, array) uses tracked allocators:
	 *
	 *   lcu_*alloc → allocation_tracker_notify_alloc → hashmap_put → lcu_*alloc → ...
	 *
	 * **When to use**:
	 * - ONLY when creating hashmap/array/container instances that back the
	 *   allocation_tracker itself (see allocation_tracker.c init functions).
	 * - Pass to hashmap_create_ex / array_new_ex as the allocator parameter.
	 *
	 * **When NOT to use**:
	 * - DO NOT use for normal application allocations.
	 * - These allocations are **invisible** to leak detection, canary checks,
	 *   and memory profiling tools.
	 *
	 * **Verification**: See P0 fix commits (89f0db9, f466916, dccc409) for the
	 * three-path recursion fix (struct/buckets/Entry) that necessitated these.
	 *
	 * @see allocation_tracker.c lines 89-142 for correct usage examples
	 */
	extern const allocator_t allocator_malloc_raw;
	extern const allocator_t allocator_calloc_raw;

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

	/**
	 * @brief Raw (untracked) allocate/free that route straight to libc malloc/free.
	 *
	 * @details These are the function-call counterparts of |allocator_malloc_raw|.
	 * They bypass the allocation tracker entirely: the returned pointer is a plain
	 * libc-malloc pointer (no canary, no tracking), and |lcu_free_raw| is a plain
	 * libc-free. Their correctness does NOT depend on whether the tracker is active.
	 *
	 * @par When to use
	 * Public APIs that allocate a buffer and TRANSFER OWNERSHIP across the library
	 * boundary (e.g. |strreplace|, |file_util_read_all|, |asprintf|, |str_params_to_str|)
	 * MUST allocate with |lcu_malloc_raw| (or libc malloc directly), so that an
	 * external caller can release it with the standard libc |free()| regardless of
	 * the tracker state. Internal lcu code (which includes mem_debug.h, so its bare
	 * |free| is rewritten to |lcu_free|) MUST release such buffers with |lcu_free_raw|
	 * to avoid an lcu_free-on-untracked-pointer abort when the tracker is active.
	 *
	 * @warning Buffers allocated here are INVISIBLE to leak detection / canary checks.
	 *          Use ONLY for cross-boundary ownership transfer, never for normal
	 *          internal allocations (those should stay tracked).
	 */
	void* lcu_malloc_raw(size_t size);
	void lcu_free_raw(void* ptr);

#ifdef __cplusplus
}
#endif

#endif // !_LCU_ALLOCATOR_H
