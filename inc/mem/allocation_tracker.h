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
 * reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/allocation_tracker.h
 *
 ******************************************************************************/
#pragma once
#ifndef LCU_ALLOCATION_TRACKER_H
#define LCU_ALLOCATION_TRACKER_H

#include <stdint.h> /* for uint8_t */
#include <stddef.h> /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct allocation_tracker_t allocation_tracker_t;
typedef uint8_t allocator_id_t;
typedef void (*report_leak_mem_fn)(void* leak_ptr, size_t leak_size, 
	char *leak_file, char *leak_func, int leak_line, void *user_data);

// Initialize the allocation tracker. If you do not call this function,
// the allocation tracker functions do nothing but are still safe to call.
void allocation_tracker_init(void);

// UnInitialize the allocation tracker. Call it if you won't alloc any memory more.
// So, call it at end of your main function.
void allocation_tracker_uninit(void);

// Reset the allocation tracker. Don't call this in the normal course of
// operations. Useful mostly for testing.
void allocation_tracker_reset(void);

// Expects that there are no allocations at the time of this call. Dumps
// information about unfreed allocations to the log. Returns the amount of
// unallocated memory. report_leak_mem_fn can be NULL.
size_t allocation_tracker_expect_no_allocations(report_leak_mem_fn fn_report, void *report_fn_user_data);

// Notify the tracker of a new allocation belonging to |allocator_id|.
// If |ptr| is NULL, this function does nothing. |requested_size| is the
// size of the allocation without any canaries. The caller must allocate
// enough memory for canaries; the total allocation size can be determined
// by calling |allocation_tracker_resize_for_canary|. Returns |ptr| offset
// to the the beginning of the uncanaried region.
void *allocation_tracker_notify_alloc(allocator_id_t allocator_id, void *ptr, size_t requested_size, 
	const char* file_path, const char* func_name, int file_line);

// Notify the tracker of an allocation that is being freed. |ptr| must be a
// pointer returned by a call to |allocation_tracker_notify_alloc| with the
// same |allocator_id|. If |ptr| is NULL, this function does nothing. Returns
// |ptr| offset to the real beginning of the allocation including any canary
// space.
void *allocation_tracker_notify_free(allocator_id_t allocator_id, void *ptr);

// Get ptr size: user requested memory size, not allocation_tracker allocated real size.
// WARN: ptr must tracked before, and not freed.
size_t allocation_tracker_ptr_size(allocator_id_t allocator_id, void* ptr);

// Get the full size for an allocation, taking into account the size of canaries.
size_t allocation_tracker_resize_for_canary(size_t size);

#ifdef __cplusplus
}
#endif

#endif // LCU_ALLOCATION_TRACKER_H
