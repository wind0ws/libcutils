#pragma once
#ifndef LCU_ARRAY_H
#define LCU_ARRAY_H

/**
 * @file array.h
 * @brief Dynamic array implementation
 * 
 * reference: https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/array.h
 */

#include <stdbool.h> /* for true/false */
#include <stddef.h>  /* for size_t     */
#include <stdint.h>  /* for uint32_t   */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef struct array_t array_t;

	/**
	 * @brief Creates a new array object that stores elements of specified size
	 * 
	 * @param[in] element_size Size of each element in bytes
	 * @return array_t* Pointer to the newly created array object, or NULL on failure
	 * @note The returned object must be freed with array_free()
	 * @note element_size must be greater than 0
	 */
	array_t* array_new(size_t element_size);

	/**
	 * @brief Creates a new array object with specified initial capacity
	 * 
	 * @param[in] element_size Size of each element in bytes
	 * @param[in] init_capacity Initial capacity for the array
	 * @return array_t* Pointer to the newly created array object, or NULL on failure
	 * @note The returned object must be freed with array_free()
	 * @note element_size must be greater than 0
	 * @note init_capacity will be used for initial data capacity
	 */
	array_t* array_new_with_init_capacity(size_t element_size, size_t init_capacity);

	/**
	 * @brief Frees an array that was allocated with array_new
	 * 
	 * @param[in] array Pointer to the array to be freed
	 * @note array may be NULL, in which case this function does nothing
	 */
	void array_free(array_t* array);

	/**
	 * @brief Returns a pointer to the first stored element in the array
	 * 
	 * @param[in] array Pointer to the array
	 * @return void* Pointer to the first element
	 * @note array must not be NULL
	 */
	void* array_ptr(const array_t* array);

	/**
	 * @brief Returns a pointer to the specified element in the array
	 * 
	 * @param[in] array Pointer to the array
	 * @param[in] index Index of the element to access
	 * @return void* Pointer to the element at the specified index
	 * @note index must be less than the array's length
	 * @note array must not be NULL
	 */
	void* array_at(const array_t* array, size_t index);

	/**
	 * @brief Returns the number of elements stored in the array
	 * 
	 * @param[in] array Pointer to the array
	 * @return size_t Number of elements in the array
	 * @note array must not be NULL
	 */
	size_t array_length(const array_t* array);

	/**
	 * @brief Inserts an element to the end of the array by value
	 * 
	 * @param[in] array Pointer to the array
	 * @param[in] value Value to insert (uint32_t)
	 * @return true if the element was successfully inserted, false on error
	 * @note For example, a caller may simply call array_append_value(array, 5) 
	 *       instead of storing 5 into a variable and then inserting by pointer
	 * @note Although value is a uint32_t, only the lowest element_size bytes will be stored
	 * @note array must not be NULL
	 */
	bool array_append_value(array_t* array, uint32_t value);

	/**
	 * @brief Inserts an element to the end of the array by pointer
	 * 
	 * @param[in] array Pointer to the array
	 * @param[in] data Pointer to the data to be inserted
	 * @return true if the element was successfully inserted, false on error
	 * @note The value pointed to by data must be at least element_size bytes long
	 * @note The data will be copied into the array
	 * @note Neither array nor data may be NULL
	 */
	bool array_append_ptr(array_t* array, void* data);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LCU_ARRAY_H
