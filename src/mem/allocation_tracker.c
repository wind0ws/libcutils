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
 ******************************************************************************/

/* CRITICAL: This file does NOT include mem_debug.h to avoid infinite recursion.
 * The allocation tracker itself cannot be tracked, as every allocation here would
 * trigger allocation_tracker_notify_alloc -> hashmap_put -> allocator -> tracker again.
 * The tracker's internal hashmap uses allocator_calloc_raw (defined in allocator.c)
 * which bypasses tracking. See hashmap_create_with_allocator call and the CRITICAL comment there. */

#include "mem/allocation_tracker.h"
#include "thread/posix_thread.h"
#include <stdbool.h>
#include "mem/strings.h"
#include "data/hash_functions.h"
#include "data/hashmap.h"
#include "common_macro.h"

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

typedef struct
{
	uint8_t allocator_id;
	void* ptr;
	/* ptr request alloc memory size */
	size_t size;
	bool freed;
#define MAX_FILE_PATH_LEN (128)
	char file_path[MAX_FILE_PATH_LEN];
#define MAX_FUNCTION_NAME_LEN (32)
	char func_name[MAX_FUNCTION_NAME_LEN];
	int file_line;
} allocation_t;

typedef struct
{
	size_t unfreed_memory_size;
	report_leak_mem_fn fn_report;
	void* report_fn_user_data;
} allocation_free_checker_context;

#define ALLOCATION_MAP_INIT_CAPACITY    (1024)

/* Fix #3: canary length is a COMPILE-TIME constant, not a runtime value set in
 * init(). Previously |canary_size| was assigned in allocation_tracker_init()
 * and left dangling after uninit(); making it constant guarantees the offset
 * arithmetic (+/- CANARY_SIZE) is identical in every phase, including any
 * stray free() that races teardown. */
#define CANARY_STR  "tinybird"
enum { CANARY_SIZE = sizeof(CANARY_STR) - 1 };
static const char* const canary = CANARY_STR;

static hashmap_t* allocations = NULL;
static pthread_mutex_t allocations_lock;

/* Fix #3: once uninit() has torn down the tracker, any later lcu_free() of a
 * once-tracked (canary-offset) pointer can no longer be un-offset safely and
 * will corrupt the heap. We cannot auto-repair it (tracked vs raw is
 * indistinguishable post-teardown), but we refuse to stay silent: warn loudly
 * so the lifecycle violation (e.g. a static dtor freeing lcu memory after
 * MEM_CHECK_DEINIT) is visible in development. */
static bool tracker_was_torn_down = false;

/* Fix #3: one-shot guard so the post-teardown free warning (in notify_free)
 * fires at most once instead of spamming every late free. */
static bool uninit_use_warned = false;

static bool allocation_entry_freed_checker(void* key, void* value, void* context);
static bool allocation_memory_corruption_checker(allocation_t* allocation);

/*
static int lock_allocations_map(void* arg)
{
	return pthread_mutex_lock((pthread_mutex_t*)arg);
}

static int unlock_allocations_map(void* arg)
{
	return pthread_mutex_unlock((pthread_mutex_t*)arg);
}
*/

static bool pointer_key_equals(const void* x, const void* y)
{
	return x == y;
}

void allocation_tracker_init(void)
{
	if (allocations)
	{
		return;
	}
	/* M4: a failed mutex init leaves every subsequent lock op undefined; that
	 * is an unrecoverable setup error, so abort early rather than limp on with
	 * a broken lock guarding the allocations map. */
	int mtx_rc = pthread_mutex_init(&allocations_lock, NULL);
	ASSERT_ABORT(0 == mtx_rc);

	hashmap_lock_t map_lock =
	{
       .arg = &allocations_lock,
       .acquire = (int(*)(void*))pthread_mutex_lock, //lock_allocations_map,
       .release = (int(*)(void*))pthread_mutex_unlock, //unlock_allocations_map,
	};
	/* CRITICAL: the tracker's internal map MUST use the raw (untracked)
	 * allocator. If it used a tracked allocator, every allocation here would
	 * recurse: lcu_*alloc -> allocation_tracker_notify_alloc -> hashmap_put
	 * -> lcu_*alloc -> ... (and would also self-deadlock on allocations_lock).
	 * Do NOT change &allocator_calloc_raw to a tracked allocator. */
	allocations = hashmap_create_with_allocator(ALLOCATION_MAP_INIT_CAPACITY,
		hash_function_pointer, NULL, free, pointer_key_equals, &map_lock,
		&allocator_calloc_raw);
}

// Test function only. Do not call in the normal course of operations.
void allocation_tracker_uninit(void)
{
	if (!allocations)
	{
		return;
	}

	/* Fix #3 (A4): report any still-live tracked allocations as a fatal-level
	 * diagnostic, then CONTINUE teardown. We deliberately do NOT abort here:
	 * a leak is not heap corruption, and turning "leak at exit" into "crash at
	 * exit" would punish otherwise-correct programs and mask unrelated issues.
	 * Callers who want hard enforcement should check
	 * allocation_tracker_expect_no_allocations()'s return value themselves. */
	size_t leaked = allocation_tracker_expect_no_allocations(NULL, NULL);
	if (leaked > 0)
	{
		lcu_diagnostics_writef("MEMORY_LEAK",
			"'%s' tearing down tracker with %zu bytes still unfreed; "
			"these blocks can no longer be validated/un-offset safely",
			__func__, leaked);
	}

	hashmap_free(allocations);
	allocations = NULL;

	pthread_mutex_destroy(&allocations_lock);

	/* Fix #3: from here on, any lcu_free() of a once-tracked pointer is a
	 * lifecycle violation we can detect but not repair. Arm the warning. */
	tracker_was_torn_down = true;
}

void allocation_tracker_reset(void)
{
	if (!allocations)
	{
		return;
	}
	hashmap_clear(allocations);
}

size_t allocation_tracker_expect_no_allocations(report_leak_mem_fn fn_report, void* report_fn_user_data)
{
	if (!allocations)
	{
		return 0;
	}
	allocation_free_checker_context context =
	{
		.unfreed_memory_size = 0,
		.fn_report = fn_report,
		.report_fn_user_data = report_fn_user_data
	};
	hashmap_foreach(allocations, allocation_entry_freed_checker, &context);
	return context.unfreed_memory_size;
}

void* allocation_tracker_notify_alloc(allocator_id_t allocator_id, void* ptr, size_t requested_size,
	const char* file_path, const char* func_name, int file_line)
{
	if (!allocations || !ptr)
	{
		return ptr;
	}
	char* return_ptr = (char*)ptr;
	return_ptr += CANARY_SIZE;

	allocation_t* allocation = (allocation_t*)hashmap_get(allocations, return_ptr);
	if (allocation)
	{
		ASSERT_ABORT(allocation->freed); // Must have been freed before
	}
	else
	{
		allocation = (allocation_t*)calloc(1, sizeof(allocation_t));
		ASSERT_ABORT(allocation);
		hashmap_put(allocations, return_ptr, allocation);
	}
	allocation->allocator_id = allocator_id;
	allocation->freed = false;
	allocation->size = requested_size;
	allocation->ptr = return_ptr;
	if (file_path)
	{
		strlcpy(allocation->file_path, file_path, MAX_FILE_PATH_LEN);
	}
	if (func_name)
	{
		strlcpy(allocation->func_name, func_name, MAX_FUNCTION_NAME_LEN);
	}
	allocation->file_line = file_line;

	// Add the canary on both sides
	memcpy(return_ptr - CANARY_SIZE, canary, CANARY_SIZE);
	memcpy(return_ptr + requested_size, canary, CANARY_SIZE);
	return return_ptr;
}

void* allocation_tracker_notify_free(allocator_id_t allocator_id, void* ptr)
{
	if (!allocations || !ptr)
	{
		/* Fix #3: tracker torn down but a (possibly once-tracked) pointer is
		 * being freed. We return |ptr| unchanged (cannot un-offset safely), but
		 * if it WAS a tracked, canary-offset pointer the caller's libc free will
		 * corrupt the heap. Warn loudly so the lifecycle violation is visible. */
		if (ptr && tracker_was_torn_down && !uninit_use_warned)
		{
			uninit_use_warned = true;
			lcu_diagnostics_writef("MEM_CHECK",
				"'%s' called after tracker teardown (addr 0x%zx) — freeing a "
				"once-tracked pointer now risks heap corruption; ensure all lcu "
				"memory is released before MEM_CHECK_DEINIT().",
				__func__, (uintptr_t)ptr);
		}
		return ptr;
	}
	allocation_t* allocation = (allocation_t*)hashmap_get(allocations, ptr);
	if (!allocation)
	{
		/* Keep mem_debug.h's free macro compatible with raw/libc pointers.
		 * lcu_free() will free the returned pointer through the raw libc path. */
		return ptr;
	}
	ASSERT_ABORT(!allocation->freed);                       // Must not double free
	ASSERT_ABORT(allocation->allocator_id == allocator_id); // Must be from the same allocator
	allocation->freed = true;
	allocation_memory_corruption_checker(allocation);
	// Free the hash map entry to avoid unlimited memory usage growth.
	// Double-free of memory is detected with "ASSERT_ABORT(allocation)" above
	// as the allocation entry will not be present.
	hashmap_remove(allocations, ptr);
	return ((char*)ptr) - CANARY_SIZE;
}

size_t allocation_tracker_ptr_size(allocator_id_t allocator_id, void* ptr)
{
	if (!allocations || !ptr)
	{
		return 0;
	}
	allocation_t* allocation = (allocation_t*)hashmap_get(allocations, ptr);
	ASSERT_ABORT(allocation);                               // Must have been tracked before
	ASSERT_ABORT(allocation->allocator_id == allocator_id); // Must be from the same allocator
	return allocation->size;
}

bool allocation_tracker_try_ptr_size(allocator_id_t allocator_id, void* ptr, size_t* out_size)
{
	/* Fix #1: non-aborting counterpart of allocation_tracker_ptr_size().
	 * Returns false (without asserting) when the tracker is inactive or |ptr|
	 * is not tracked, so lcu_realloc_trace can fall back to libc realloc for
	 * raw/cross-boundary pointers — symmetric with notify_free's raw fallback. */
	if (out_size)
	{
		*out_size = 0;
	}
	if (!allocations || !ptr)
	{
		return false;
	}
	allocation_t* allocation = (allocation_t*)hashmap_get(allocations, ptr);
	if (!allocation)
	{
		return false;
	}
	/* A tracked pointer from a different allocator id is still a misuse worth
	 * catching; keep the hard assert for that genuine programming error. */
	ASSERT_ABORT(allocation->allocator_id == allocator_id);
	if (out_size)
	{
		*out_size = allocation->size;
	}
	return true;
}

size_t allocation_tracker_resize_for_canary(size_t size)
{
	if (!allocations)
	{
		return size;
	}
	/* C-1 修复: 防止 size + 2*CANARY_SIZE 溢出。
	 * 当 size > SIZE_MAX - 2*CANARY_SIZE 时，加法会环绕为极小值，
	 * 导致分配小堆块但尾 canary 越界写入后续内存。
	 * CANARY_SIZE = 8 (strlen("tinybird"))，保守检查 16 字节余量。 */
	if (size > SIZE_MAX - (2 * CANARY_SIZE))
	{
		return 0;  /* 返回 0 通知调用方溢出（size != 0 时 0 是非法值） */
	}
	return size + (2 * CANARY_SIZE);
}

static bool allocation_memory_corruption_checker(allocation_t* allocation)
{
	void* ptr = allocation->ptr;
	UNUSED_ATTR const char* beginning_canary = ((char*)ptr) - CANARY_SIZE;
	UNUSED_ATTR const char* end_canary = ((char*)ptr) + allocation->size;
	for (size_t i = 0; i < CANARY_SIZE; ++i)
	{
		if (beginning_canary[i] != canary[i] ||
			end_canary[i] != canary[i])
		{
			lcu_diagnostics_fatalf("MEMORY_CORRUPTION",
				"detected corrupted memory at '%s' (%s:%d), address: 0x%zx, size: %zd bytes",
				NULLABLE_STRING(allocation->func_name), NULLABLE_STRING(allocation->file_path), 
				allocation->file_line, (uintptr_t)allocation->ptr, allocation->size);
			return false;
		}
	}
	return true;
}

static bool allocation_entry_freed_checker(void *key, void *value, void* context)
{
	allocation_t* allocation = (allocation_t*)value;
	if (!allocation->freed)
	{
		allocation_free_checker_context* checker_ctx = (allocation_free_checker_context*)context;
		checker_ctx->unfreed_memory_size += allocation->size; // Report back the unfreed byte count
		allocation_memory_corruption_checker(allocation);
		if (checker_ctx->fn_report)
		{
			checker_ctx->fn_report(allocation->ptr, allocation->size,
				allocation->file_path, allocation->func_name, allocation->file_line,
				checker_ctx->report_fn_user_data);
		}
		else
		{
			lcu_diagnostics_writef("MEMORY_LEAK",
				"'%s' found unfreed memory at '%s' (%s:%d), address: 0x%zx size: %zd bytes", __func__,
				NULLABLE_STRING(allocation->func_name), NULLABLE_STRING(allocation->file_path), allocation->file_line,
				(uintptr_t)allocation->ptr, allocation->size);
		}
	}
	return true;
}
