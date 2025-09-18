/******************************************************************************
 *  Implementation of hashmap.
 *  reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/hash_map.h
 ******************************************************************************/

 // Attention: HashMap is NOT thread safe!
 // for concurrency support, you should provide lock/unlock function on hashmap_create. 
 // or protect all operations on hashmap by your self.

#pragma once
#ifndef LCU_HASHMAP_H
#define LCU_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * @brief Concurrency support structure.
	 */
	typedef struct hashmap_lock
	{
		void* arg; /**< Argument to be passed to acquire and release function pointers */
		int (*acquire)(void* arg); /**< Function pointer to acquire a lock */
		int (*release)(void* arg); /**< Function pointer to release a lock */
	} hashmap_lock_t;

	/**
	 * @brief Hash key function prototype
	 * @param key The key to be hashed
	 * @return Hash value of the key
	 */
	typedef int (*hash_key_fn)(const void* key);
	
	/**
	 * @brief Key equality function prototype
	 * @param x First key to compare
	 * @param y Second key to compare
	 * @return true if keys are equal, false otherwise
	 */
	typedef bool (*key_equality_fn)(const void* x, const void* y);
	
	/**
	 * @brief Free key pointer function prototype
	 * @param key Key to be freed
	 */
	typedef void (*key_free_fn)(void* key);
	
	/**
	 * @brief Free value pointer function prototype
	 * @param data Value to be freed
	 */
	typedef void (*value_free_fn)(void* data);
	
	/**
	 * @brief Hashmap foreach iterator callback prototype
	 * @param key Current key being iterated
	 * @param value Current value being iterated
	 * @param context User-defined context passed to the callback
	 * @return true to continue iteration, false to stop
	 */
	typedef bool (*hashmap_iter_cb)(void* key, void* value, void* context);

	/**
	 * @brief A hash map data structure
	 */
	typedef struct Hashmap hashmap_t;

	/**
	 * @brief Creates a new hash map
	 * 
	 * @param initial_capacity Number of expected entries
	 * @param fn_hash Function which hashes keys
	 * @param fn_key_free Function which free keys when remove
	 * @param fn_value_free Function which free values when remove
	 * @param fn_key_equality Function which compares keys for equality
	 * @param lock Provide lock/unlock function for concurrency support
	 * 
	 * @return Hashmap pointer, or NULL if memory allocation fails. 
	 *         You should call hashmap_free after use!
	 */
	hashmap_t* hashmap_create(size_t initial_capacity,
		hash_key_fn fn_hash,
		key_free_fn fn_key_free,
		value_free_fn fn_value_free,
		key_equality_fn fn_key_equality,
		hashmap_lock_t* lock);

	/**
	 * @brief Frees the hash map
	 * 
	 * If you set key_free_fn and value_free_fn, we will call it on free,
	 * so keys/values can be freed automatically;
	 * otherwise it does not free the keys or values themselves, you should do it by yourself.
	 * 
	 * @param map Hashmap to be freed
	 */
	void hashmap_free(hashmap_t* map);

	/**
	 * @brief Hashes the memory pointed to by key with the given size
	 * 
	 * Useful for implementing hash functions.
	 * 
	 * @param key Pointer to the memory to hash
	 * @param keySize Size of the memory to hash
	 * @return Hash value
	 */
	int hashmap_hash(void* key, size_t keySize);

	/**
	 * @brief Get current hashmap size
	 * 
	 * @param map Hashmap to get size from
	 * @return Number of key-value pairs in the hashmap
	 */
	size_t hashmap_size(hashmap_t* map);

	/**
	 * @brief Puts value for the given key in the map
	 * 
	 * @param map Hashmap to put value into
	 * @param key Key to associate with value
	 * @param value Value to store in the map
	 * @return Pre-existing value if any, or NULL if no previous value existed
	 * 
	 * @warning The return value may no longer be usable if you set value_free_fn.
	 *          For more details, see hashmap_remove warnings.
	 * 
	 * @note If memory allocation fails, this function returns NULL,
	 *       the map's size does not increase, and errno is set to ENOMEM.
	 */
	void* hashmap_put(hashmap_t* map, void* key, void* value);

	/**
	 * @brief Gets a value from the map
	 * 
	 * @param map Hashmap to get value from
	 * @param key Key to look up
	 * @return Value associated with the key, or NULL if no entry is found
	 *         or if the value itself is NULL
	 */
	void* hashmap_get(hashmap_t* map, void* key);

	/**
	 * @brief Removes an entry from the map
	 * 
	 * @param map Hashmap to remove entry from
	 * @param key Key of the entry to remove
	 * @return The removed value, or NULL if no entry was present
	 * 
	 * @warning If you set fn_value_free function, we will call it on remove.
	 *          For example: fn_value_free = free, after this remove function is called,
	 *          the return value is no longer usable because it has been freed!
	 *          You can only compare the return value with NULL and nothing else.
	 */
	void* hashmap_remove(hashmap_t* map, void* key);

	/**
	 * @brief Removes all elements in the hashmap
	 * 
	 * Calling this function will return the hashmap to the same state 
	 * it was in after hashmap_create.
	 * 
	 * @param map Hashmap to clear, may not be NULL
	 */
	void hashmap_clear(hashmap_t* map);

	/**
	 * @brief Invokes the given callback on each entry in the map
	 * 
	 * @param map Hashmap to iterate over
	 * @param callback Function to call for each entry
	 * @param context User-defined context passed to the callback
	 * 
	 * @note Stops iterating if the callback returns false
	 */
	void hashmap_foreach(hashmap_t* map, hashmap_iter_cb callback, void* context);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LCU_HASHMAP_H
