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
#define LOG_TAG "allocation_tracker"

#include "lcu_stdafx.h"
#include "mem/allocation_tracker.h"
 //#include <assert.h>
 //#include <pthread.h>
#include "thread/thread_wrapper.h"
#include <stdlib.h>
#include <string.h>
#include "mem/allocator.h"
#include "data/hash_functions.h"
#include "data/hash_map.h"
#include "log/xlog.h"
#include "common_macro.h"

typedef struct 
{
	uint8_t allocator_id;
	void* ptr;
	size_t size;
	bool freed;
} allocation_t;

typedef struct
{
	size_t unfreed_memory_size;
	report_leak_mem_fn fn_report;
} allocation_free_checker_context;

// Hidden constructor for hash map for our use only. 
//Everything else should use the normal interface.
hash_map_t* hash_map_new_internal(
	size_t size,
	hash_index_fn hash_fn,
	key_free_fn key_fn,
	data_free_fn,
	key_equality_fn equality_fn,
	const allocator_t* zeroed_allocator);

static bool allocation_entry_freed_checker(hash_map_entry_t* entry, void* context);

static inline void* untracked_calloc(size_t size);

static const size_t allocation_hash_map_size = 1024;
static const char* canary = "tinybird";
static const allocator_t untracked_calloc_allocator = 
{
  untracked_calloc,
  free
};
static size_t canary_size = 0;
static hash_map_t* allocations = NULL;
static pthread_mutex_t lock;

void allocation_tracker_init(void) 
{
	if (allocations)
		return;
	canary_size = strlen(canary);
	pthread_mutex_init(&lock, NULL);
	pthread_mutex_lock(&lock);
	allocations = hash_map_new_internal(
		allocation_hash_map_size,
		hash_function_pointer,
		NULL,
		free,
		NULL,
		&untracked_calloc_allocator);
	pthread_mutex_unlock(&lock);
}

// Test function only. Do not call in the normal course of operations.
void allocation_tracker_uninit(void) 
{
	if (!allocations)
		return;
	pthread_mutex_lock(&lock);
	hash_map_free(allocations);
	allocations = NULL;
	pthread_mutex_unlock(&lock);

	pthread_mutex_destroy(&lock);
}

void allocation_tracker_reset(void) 
{
	if (!allocations)
		return;
	pthread_mutex_lock(&lock);
	hash_map_clear(allocations);
	pthread_mutex_unlock(&lock);
}

size_t allocation_tracker_expect_no_allocations(report_leak_mem_fn fn_report) 
{
	if (!allocations)
		return 0;
	
	allocation_free_checker_context context = 
	{
		.unfreed_memory_size = 0,
		.fn_report = fn_report
	};
	pthread_mutex_lock(&lock);
	hash_map_foreach(allocations, allocation_entry_freed_checker, &context);
	pthread_mutex_unlock(&lock);

	return context.unfreed_memory_size;
}

void* allocation_tracker_notify_alloc(allocator_id_t allocator_id, void* ptr, size_t requested_size) 
{
	if (!allocations || !ptr)
		return ptr;
	char* return_ptr = (char*)ptr;
	return_ptr += canary_size;

	pthread_mutex_lock(&lock);
	allocation_t* allocation = (allocation_t*)hash_map_get(allocations, return_ptr);
	if (allocation) 
	{
		ASSERT_ABORT(allocation->freed); // Must have been freed before
	}
	else 
	{
		allocation = (allocation_t*)calloc(1, sizeof(allocation_t));
		ASSERT_ABORT(allocation);
		hash_map_set(allocations, return_ptr, allocation);
	}
	pthread_mutex_unlock(&lock);
	allocation->allocator_id = allocator_id;
	allocation->freed = false;
	allocation->size = requested_size;
	allocation->ptr = return_ptr;

	// Add the canary on both sides
	memcpy(return_ptr - canary_size, canary, canary_size);
	memcpy(return_ptr + requested_size, canary, canary_size);
	return return_ptr;
}

void* allocation_tracker_notify_free(allocator_id_t allocator_id, void* ptr) 
{
	if (!allocations || !ptr)
		return ptr;
	pthread_mutex_lock(&lock);
	allocation_t* allocation = (allocation_t*)hash_map_get(allocations, ptr);
	ASSERT_ABORT(allocation);                               // Must have been tracked before
	ASSERT_ABORT(!allocation->freed);                       // Must not be a double free
	ASSERT_ABORT(allocation->allocator_id == allocator_id); // Must be from the same allocator
	allocation->freed = true;
	UNUSED_ATTR const char* beginning_canary = ((char*)ptr) - canary_size;
	UNUSED_ATTR const char* end_canary = ((char*)ptr) + allocation->size;
	for (size_t i = 0; i < canary_size; i++) 
	{
		ASSERT_ABORT(beginning_canary[i] == canary[i]);
		ASSERT_ABORT(end_canary[i] == canary[i]);
	}
	// Free the hash map entry to avoid unlimited memory usage growth.
	// Double-free of memory is detected with "ASSERT_ABORT(allocation)" above
	// as the allocation entry will not be present.
	hash_map_erase(allocations, ptr);
	pthread_mutex_unlock(&lock);
	return ((char*)ptr) - canary_size;
}

size_t allocation_tracker_ptr_size(allocator_id_t allocator_id, void* ptr) 
{
	if (!allocations || !ptr)
		return 0;
	size_t ptr_size = 0;
	pthread_mutex_lock(&lock);
	allocation_t* allocation = (allocation_t*)hash_map_get(allocations, ptr);
	pthread_mutex_unlock(&lock);
	ASSERT_ABORT(allocation);                               // Must have been tracked before
	ASSERT_ABORT(allocation->allocator_id == allocator_id); // Must be from the same allocator
	ptr_size = allocation->size;

	return ptr_size;
}

size_t allocation_tracker_resize_for_canary(size_t size) 
{
	return (!allocations) ? size : size + (2 * canary_size);
}

static bool allocation_entry_freed_checker(hash_map_entry_t* entry, void* context) 
{
	allocation_t* allocation = (allocation_t*)entry->data;
	if (!allocation->freed) 
	{
		allocation_free_checker_context* checker_ctx = (allocation_free_checker_context*)context;
		checker_ctx->unfreed_memory_size += allocation->size; // Report back the unfreed byte count
		TLOGE(LOG_TAG, "%s found unfreed allocation. address: 0x%zx size: %zd bytes", __func__, (uintptr_t)allocation->ptr, allocation->size);
		if (checker_ctx->fn_report)
		{
			checker_ctx->fn_report(allocation->ptr, allocation->size);
		}
	}
	return true;
}

static inline void* untracked_calloc(size_t size) 
{
	return calloc(size, 1);
}