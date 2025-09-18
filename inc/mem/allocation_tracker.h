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
 * @return Original memory pointer including guard regions
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
 * @brief Calculates total allocation size including guard regions
 * @param[in] size User-requested allocation size
 * @return Total allocation size including front/back guard regions
 */
size_t allocation_tracker_resize_for_canary(size_t size);

#ifdef __cplusplus
}
#endif

#endif // LCU_ALLOCATION_TRACKER_H
