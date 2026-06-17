/**
 * @file allocation_tracker.h
 * @brief Memory Allocation Tracker Interface
 * 
 * Provides memory allocation tracking functionality for debugging memory leaks and memory corruption detection
 */
#pragma once
#ifndef LCU_ALLOCATION_TRACKER_H
#define LCU_ALLOCATION_TRACKER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct allocation_tracker_t allocation_tracker_t;
typedef uint8_t allocator_id_t;

typedef void (*report_leak_mem_fn)(void* leak_ptr, size_t leak_size, 
    char *leak_file, char *leak_func, int leak_line, void *user_data);

/**
 * @brief Initializes the memory allocation tracker
 * @note If not called, tracking remains in a safe but inactive state
 */
void allocation_tracker_init(void);

/**
 * @brief Deinitializes the memory allocation tracker
 * @warning Should be called before program exit to ensure proper resource release
 *
 * @warning LIFECYCLE CONTRACT: ALL memory allocated through the tracked path
 *          (lcu_malloc/calloc/strdup/realloc and the C++ new under memcheck)
 *          MUST be freed BEFORE this runs. A tracked pointer is canary-offset;
 *          once the tracker is gone the offset can no longer be undone, so a
 *          later free() of such a pointer (e.g. from a static/global object's
 *          destructor sequenced after MEM_CHECK_DEINIT) frees the wrong base
 *          address and corrupts the heap. Surviving allocations are reported as
 *          a fatal-level MEMORY_LEAK at teardown, and any post-teardown free is
 *          warned about, but neither can be auto-repaired — do not rely on them.
 *
 * @warning (M1) NOT thread-safe against concurrent alloc/free. uninit() frees
 *          the internal map and destroys its mutex without holding a lock that
 *          the notify_* paths also take; a notify_alloc/free racing uninit can
 *          touch a freed map (use-after-free). This is a TEST-ONLY entry point:
 *          quiesce all threads that allocate/free through the tracker before
 *          calling it.
 */
void allocation_tracker_uninit(void);

/**
 * @brief Resets the memory allocation tracker state
 * @warning For testing environments only - avoid use in normal operations
 */
void allocation_tracker_reset(void);

/**
 * @brief Checks for unreleased memory allocations
 * @param[in] fn_report Callback function when leaks are found (nullable)
 * @param[in] report_fn_user_data User data for callback function
 * @return Total bytes of unreleased memory
 */
size_t allocation_tracker_expect_no_allocations(report_leak_mem_fn fn_report, void *report_fn_user_data);

/**
 * @brief Tracks a newly allocated memory block
 * @param[in] allocator_id Allocator identifier
 * @param[in] ptr Allocated memory pointer
 * @param[in] requested_size Requested allocation size (excluding guard regions)
 * @param[in] file_path Source file path where allocation occurred
 * @param[in] func_name Function name where allocation occurred
 * @param[in] file_line Source code line number where allocation occurred
 * @return Actual memory pointer with added guard regions
 */
void *allocation_tracker_notify_alloc(allocator_id_t allocator_id, void *ptr, size_t requested_size, 
    const char* file_path, const char* func_name, int file_line);

/**
 * @brief Releases a tracked memory block
 * @param[in] allocator_id Allocator identifier
 * @param[in] ptr Memory pointer to free
 * @return Original memory pointer including guard regions for tracked pointers.
 *         If the tracker is active but |ptr| is not tracked, returns |ptr|
 *         unchanged so mem_debug.h's free macro remains compatible with raw
 *         libc pointers returned by public APIs.
 */
void *allocation_tracker_notify_free(allocator_id_t allocator_id, void *ptr);

/**
 * @brief Gets user-requested size of memory block
 * @param[in] allocator_id Allocator identifier
 * @param[in] ptr Tracked memory pointer
 * @return User-requested allocation size (0 for invalid pointers)
 */
size_t allocation_tracker_ptr_size(allocator_id_t allocator_id, void* ptr);

/**
 * @brief Non-aborting query of a tracked block's user-requested size.
 * @param[in]  allocator_id Allocator identifier
 * @param[in]  ptr          Pointer to query
 * @param[out] out_size     Receives the user-requested size on success (set to
 *                          0 on failure); may be NULL.
 * @return true if |ptr| is currently tracked (and *out_size is its size);
 *         false if the tracker is inactive or |ptr| is untracked. Unlike
 *         allocation_tracker_ptr_size(), this NEVER asserts on an untracked
 *         pointer, so callers (lcu_realloc_trace) can fall back to libc for
 *         raw / cross-boundary pointers — symmetric with notify_free.
 */
bool allocation_tracker_try_ptr_size(allocator_id_t allocator_id, void* ptr, size_t* out_size);

/**
 * @brief Calculates total allocation size including guard regions
 * @param[in] size User-requested allocation size
 * @return Total allocation size including front/back guard regions
 */
size_t allocation_tracker_resize_for_canary(size_t size);

#ifdef __cplusplus
}
#endif

#endif // LCU_ALLOCATION_TRACKER_H
