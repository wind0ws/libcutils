#pragma once
#ifndef LCU_ARRAY_H
#define LCU_ARRAY_H

/*
* reference: https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/array.h
*/

#include <stdbool.h> /* for true/false */
#include <stddef.h>  /* for size_t     */
#include <stdint.h>  /* for uint32_t   */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef struct array_t array_t;

	// Returns a new array object that stores elements of size |element_size|. The returned
	// object must be freed with |array_free|. |element_size| must be greater than 0. Returns
	// NULL on failure.
	array_t* array_new(size_t element_size);

	// Returns a new array object that stores elements of size |element_size|. The returned
	// object must be freed with |array_free|. |element_size| must be greater than 0. Returns
	// NULL on failure. init_capacity will used for init data capacity.
	array_t* array_new_with_init_capacity(size_t element_size, size_t init_capacity);

	// Frees an array that was allocated with |array_new|. |array| may be NULL.
	void array_free(array_t* array);

	// Returns a pointer to the first stored element in |array|. |array| must not be NULL.
	void* array_ptr(const array_t* array);

	// Returns a pointer to the |index|th element of |array|. |index| must be less than
	// the array's length. |array| must not be NULL.
	void* array_at(const array_t* array, size_t index);

	// Returns the number of elements stored in |array|. |array| must not be NULL.
	size_t array_length(const array_t* array);

	// Inserts an element to the end of |array| by value. For example, a caller
	// may simply call array_append_value(array, 5) instead of storing 5 into a
	// variable and then inserting by pointer. Although |value| is a uint32_t,
	// only the lowest |element_size| bytes will be stored. |array| must not be
	// NULL. Returns true if the element could be inserted into the array, false
	// on error.
	bool array_append_value(array_t* array, uint32_t value);

	// Inserts an element to the end of |array|. The value pointed to by |data| must
	// be at least |element_size| bytes long and will be copied into the array. Neither
	// |array| nor |data| may be NULL. Returns true if the element could be inserted into
	// the array, false on error.
	bool array_append_ptr(array_t* array, void* data);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LCU_ARRAY_H
