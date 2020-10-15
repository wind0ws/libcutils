/******************************************************************************
 *  Implementation of hashmap.
 *  reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/hash_map.h
 ******************************************************************************/

 // Attention: HashMap is NOT thread safe!
 // for concurrency support, you should add lock/unlock function on create 
 // or protect all operations of hashmap by your self.

#pragma once
#ifndef LCU_HASHMAP_H
#define LCU_HASHMAP_H

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

	//hash key function prototype
	typedef int (*hash_key_fn)(const void* key);
	//key equality function prototype
	typedef bool (*key_equality_fn)(const void* x, const void* y);
	//free key pointer function prototype
	typedef void (*key_free_fn)(void* key);
	//free value pointer function prototype
	typedef void (*value_free_fn)(void* data);
	//hashmap foreach iterator callback prototype
	typedef bool (*hashmap_iter_cb)(void* key, void* value, void* context);

	/** A hash map. */
	typedef struct Hashmap hashmap_t;

	/**
	 * Creates a new hash map. Returns NULL if memory allocation fails.
	 *
	 * @param initial_capacity number of expected entries
	 * @param fn_hash function which hashes keys
	 * @param fn_key_free function which free keys when remove
	 * @param fn_value_free function which free values when remove
	 * @param fn_key_equality function which compares keys for equality
	 * @param lock provide lock/unlock function for concurrency support
	 *
	 * @return hashmap_t pointer. you should call hashmap_free after use!
	 */
	hashmap_t* hashmap_create(size_t initial_capacity,
		hash_key_fn fn_hash,
		key_free_fn fn_key_free,
		value_free_fn fn_value_free,
		key_equality_fn fn_key_equality,
		hashmap_lock_t* lock);

	/**
	 * Frees the hash map.
	 * if you set key_free_fn and value_free_fn, we will call it on free,
	 * so keys/values can be freed automatically;
	 * otherwise it does not free the keys or values themselves.
	 */
	void hashmap_free(hashmap_t* map);

	/**
	 * Hashes the memory pointed to by key with the given size.
	 * Useful for implementing hash functions.
	 */
	int hashmap_hash(void* key, size_t keySize);

	/**
	 * Get current hashmap size.
	 */
	size_t hashmap_size(hashmap_t* map);

	/**
	 * Puts value for the given key in the map.
	 * Returns pre-existing value if any.
	 *
	 * warning: the return value may no longer to use if you set value_free_fn.
	 *          for more details, see hashmap_remove warnings.
	 *
	 * If memory allocation fails, this function returns NULL,
	 * the map's size does not increase, and errno is set to ENOMEM.
	 *
	 */
	void* hashmap_put(hashmap_t* map, void* key, void* value);

	/**
	 * Gets a value from the map.
	 * Returns NULL if no entry for the given key is found
	 * or if the value itself is NULL.
	 */
	void* hashmap_get(hashmap_t* map, void* key);

	/**
	 * Removes an entry from the map. Returns the removed value or NULL
	 * if no entry was present.
	 *
	 * warning: if you set fn_value_free function, we will call it on remove.
	 *          for example: fn_value_free = free, after this remove function called,
	 *          the return value is no longer to use, because it already freed!!!
	 *          you just can compare return value with NULL and nothing else.
	 *
	 */
	void* hashmap_remove(hashmap_t* map, void* key);

	/**
	 * Removes all elements in the hashmap. Calling this function will return the hashmap
	 * to the same state it was in after |hashmap_create|. |hashmap| may not be NULL.
	 */
	void hashmap_clear(hashmap_t* map);

	/**
	 * Invokes the given callback on each entry in the map.
	 * Stops iterating if the callback returns false.
	 */
	void hashmap_foreach(hashmap_t* map, hashmap_iter_cb callback, void* context);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LCU_HASHMAP_H
