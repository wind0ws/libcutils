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
 *  reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/hash_map.h
 ******************************************************************************/

 // Attention: HashMap is NOT thread safe!

#pragma once
#ifndef __HASHMAP_H_
#define __HASHMAP_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * Concurrency support.
	 */
	typedef struct hashmap_lock
	{
		void* arg; /**< Argument to be passed to acquire and release function pointers */
		int (*acquire)(void* arg); /**< Function pointer to acquire a lock */
		int (*release)(void* arg); /**< Function pointer to release a lock */
	} hashmap_lock_t;

	typedef int (*hash_key_fn)(const void* key);
	typedef bool (*key_equality_fn)(const void* x, const void* y);
	typedef void (*key_free_fn)(void* key);
	typedef void (*value_free_fn)(void* data);
	typedef bool (*hashmap_iter_cb)(void* key, void* value, void* context);

	/** A hash map. */
	typedef struct Hashmap hashmap_t;

	/**
	 * Creates a new hash map. Returns NULL if memory allocation fails.
	 *
	 * @param initialCapacity number of expected entries
	 * @param hash function which hashes keys
	 * @param equals function which compares keys for equality
	 * @param lock provide lock/unlock function for concurrency support. 
	 */
	hashmap_t* hashmap_create(size_t initial_capacity,
		hash_key_fn fn_hash,
		key_free_fn fn_key_free,
		value_free_fn fn_value_free,
		key_equality_fn fn_key_equality,
		hashmap_lock_t* lock);

	/**
	 * Frees the hash map. Does not free the keys or values themselves.
	 */
	void hashmap_free(hashmap_t* map);

	/**
	 * Hashes the memory pointed to by key with the given size. Useful for
	 * implementing hash functions.
	 */
	int hashmap_hash(void* key, size_t keySize);

	/**
	 * Get current hashmap size.
	 */
	size_t hashmap_size(hashmap_t* map);

	/**
	 * Puts value for the given key in the map. Returns pre-existing value if
	 * any.
	 *
	 * If memory allocation fails, this function returns NULL, the map's size
	 * does not increase, and errno is set to ENOMEM.
	 */
	void* hashmap_put(hashmap_t* map, void* key, void* value);

	/**
	 * Gets a value from the map. Returns NULL if no entry for the given key is
	 * found or if the value itself is NULL.
	 */
	void* hashmap_get(hashmap_t* map, void* key);

	/**
	 * Removes an entry from the map. Returns the removed value or NULL if no
	 * entry was present.
	 */
	void* hashmap_remove(hashmap_t* map, void* key);

	/**
	 * Removes all elements in the hashmap. Calling this function will return the hashmap
	 * to the same state it was in after |hashmap_create|. |hashmap| may not be NULL.
	 */
	void hashmap_clear(hashmap_t* map);

	/**
	 * Invokes the given callback on each entry in the map. Stops iterating if
	 * the callback returns false.
	 */
	void hashmap_foreach(hashmap_t* map, hashmap_iter_cb callback, void* context);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //__HASHMAP_H_