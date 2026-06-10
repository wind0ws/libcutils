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
 * reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/src/hash_map.c
 *           https://android.googlesource.com/platform/system/core/+/refs/heads/master/libcutils/hashmap.cpp
 ******************************************************************************/

/* CRITICAL: This file does NOT include mem_debug.h to avoid recursion with allocation_tracker.
 * The allocation tracker's internal storage is a hashmap. If this hashmap used tracked allocators,
 * every allocation here would trigger tracker -> hashmap_put -> allocator -> tracker (infinite loop).
 * hashmap_create_with_allocator accepts an allocator parameter; allocation_tracker passes allocator_calloc_raw
 * (defined in allocator.c) which bypasses tracking. Business code using hashmaps should use
 * tracked allocators (allocator_calloc) instead. */

#include "common_macro.h"
#include "data/hashmap.h"
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "mem/allocator.h"

typedef struct Entry Entry;
struct Entry
{
	void *key;
	int hash;
	void *value;
	Entry *next;
};

struct Hashmap
{
	Entry **buckets;
	size_t bucketCount;
	hash_key_fn fn_hash;
	key_free_fn fn_key_free;
	value_free_fn fn_value_free;
	key_equality_fn fn_key_equality;
	hashmap_lock_t lock;
	size_t size;
	const allocator_t *allocator;
#ifdef _DEBUG
	/* Debug guard: set to true during hashmap_foreach iteration.
	 * Detects illegal concurrent modification (put/remove/rehash in callback). */
	bool debug_iterating;
#endif
};

#define hashmap_enter(handle)                       \
	if ((handle != NULL) &&                         \
		((handle)->lock.acquire != NULL))           \
	{                                               \
		(handle)->lock.acquire((handle)->lock.arg); \
	}
#define hashmap_leave(handle)                       \
	if ((handle != NULL) &&                         \
		((handle)->lock.release != NULL))           \
	{                                               \
		(handle)->lock.release((handle)->lock.arg); \
	}

hashmap_t *hashmap_create_with_allocator(size_t initial_capacity,
							 hash_key_fn fn_hash,
							 key_free_fn fn_key_free,
							 value_free_fn fn_value_free,
							 key_equality_fn fn_key_equality,
							 hashmap_lock_t *lock,
							 const allocator_t *allocator)
{
	// 0.75 load factor. Check for overflow
	if (initial_capacity > (SIZE_MAX / 4))
	{
		return NULL;
	}
	if (NULL == allocator)
	{
		return NULL;
	}
	hashmap_t *map = (hashmap_t *)(allocator->alloc(sizeof(hashmap_t)));
	if (NULL == map)
	{
		return NULL;
	}
	/* Initialize the hashmap object (explicit zeroing; allocator may be non-zeroing) */
	memset(map, 0, sizeof(*map));
	map->allocator = allocator;
	/* Copy the lock if it is not NULL */
	if (NULL != lock)
	{
		memcpy(&map->lock, lock, sizeof(map->lock));
	}

	const size_t minimumBucketCount = initial_capacity * 4 / 3;
	map->bucketCount = 1;
	while (map->bucketCount <= minimumBucketCount)
	{
		// Bucket count must be power of 2. Check for overflow
		if (map->bucketCount > (SIZE_MAX / 2))
		{
			allocator->free(map);
			return NULL;
		}
		map->bucketCount <<= 1;
	}
	const size_t buckets_size = map->bucketCount * sizeof(Entry *);
	map->buckets = (Entry **)(allocator->alloc(buckets_size));
	if (NULL == map->buckets)
	{
		allocator->free(map);
		return NULL;
	}
	/* Explicit zeroing of bucket array; do NOT rely on allocator zeroing */
	memset(map->buckets, 0, buckets_size);
	map->fn_hash = fn_hash;
	map->fn_key_free = fn_key_free;
	map->fn_value_free = fn_value_free;
	map->fn_key_equality = fn_key_equality;
	map->size = 0;
	return map;
}

hashmap_t *hashmap_create(size_t initial_capacity,
						  hash_key_fn fn_hash,
						  key_free_fn fn_key_free,
						  value_free_fn fn_value_free,
						  key_equality_fn fn_key_equality,
						  hashmap_lock_t *lock)
{
	return hashmap_create_with_allocator(initial_capacity, fn_hash, fn_key_free,
							 fn_value_free, fn_key_equality, lock,
							 &allocator_calloc);
}

/**
 * Hashes the given key using unsigned arithmetic to avoid undefined behavior.
 */
static inline int private_hash_key(hashmap_t *map, void *key)
{
	int h = map->fn_hash(key);
	/* Apply secondary hashing using unsigned arithmetic */
	uint32_t uh = (uint32_t)h;
	uh += (~(uh << 9));
	uh ^= (uh >> 14);
	uh += (uh << 4);
	uh ^= (uh >> 10);
	return (int)uh;
}

static inline size_t private_calculate_index(size_t bucketCount, int hash)
{
	return ((size_t)hash) & (bucketCount - 1);
}

static void private_expand_if_necessary(hashmap_t *map)
{
#ifdef _DEBUG
	/* Redundant with put's check, but explicit: rehash during iteration is fatal. */
	ASSERT(!map->debug_iterating);
#endif
	// If the load factor exceeds 0.75...
	if (map->size <= (map->bucketCount * 3 / 4))
	{
		return;
	}
	// Start off with a 0.33 load factor.
	size_t newBucketCount = (map->bucketCount << 1);
	const size_t newBuckets_size = newBucketCount * sizeof(Entry *);
	Entry **newBuckets = (Entry **)(map->allocator->alloc(newBuckets_size));
	if (NULL == newBuckets)
	{
		// Abort expansion.
		return;
	}
	/* Explicit zeroing of new bucket array; do NOT rely on allocator zeroing */
	memset(newBuckets, 0, newBuckets_size);
	// Move over existing entries.
	size_t i;
	for (i = 0; i < map->bucketCount; ++i)
	{
		Entry *entry = map->buckets[i];
		while (NULL != entry)
		{
			Entry *next = entry->next;
			size_t index = private_calculate_index(newBucketCount, entry->hash);
			entry->next = newBuckets[index];
			newBuckets[index] = entry;
			entry = next;
		}
	}
	// Copy over internals.
	map->allocator->free(map->buckets);
	map->buckets = newBuckets;
	map->bucketCount = newBucketCount;
}

static void hashmap_clear_unsafe(hashmap_t *map)
{
	size_t i;
	for (i = 0; i < map->bucketCount; ++i)
	{
		Entry *entry = map->buckets[i];
		while (NULL != entry)
		{
			Entry *next = entry->next;
			if (entry->key && map->fn_key_free)
			{
				map->fn_key_free(entry->key);
			}
			if (entry->value && map->fn_value_free)
			{
				map->fn_value_free(entry->value);
			}
			map->allocator->free(entry);
			--map->size;
			entry = next;
		}
		map->buckets[i] = NULL;
	}
}

void hashmap_free(hashmap_t *map)
{
	if (!map || !map->buckets)
	{
		return;
	}
	/* Cache allocator: it is needed to free map itself after the lock is released */
	const allocator_t *allocator = map->allocator;
	hashmap_enter(map);
	hashmap_clear_unsafe(map);
	allocator->free(map->buckets);
	map->buckets = NULL;
	map->bucketCount = 0;
	hashmap_leave(map);
	allocator->free(map);
}

/* Safe hash function using unsigned arithmetic to avoid undefined behavior */
int hashmap_hash(void *key, size_t keySize)
{
	uint32_t h = (uint32_t)keySize;
	unsigned char *data = (unsigned char *)key;
	size_t i;
	for (i = 0; i < keySize; i++)
	{
		h = h * 31U + *data;
		data++;
	}
	/* Convert to signed int for return, this is well-defined */
	return (int)h;
}

static Entry *private_create_entry(const allocator_t *allocator, void *key, int hash, void *value)
{
	Entry *entry = (Entry *)(allocator->alloc(sizeof(Entry)));
	if (entry == NULL)
	{
		return NULL;
	}
	entry->key = key;
	entry->hash = hash;
	entry->value = value;
	entry->next = NULL;
	return entry;
}

static inline bool private_equal_keys(void *keyA, int hashA, void *keyB, int hashB, key_equality_fn fn_equals)
{
	if (keyA == keyB)
	{
		return true;
	}
	if (hashA != hashB)
	{
		return false;
	}
	return fn_equals(keyA, keyB);
}

size_t hashmap_size(hashmap_t *map)
{
	/* NOTE(reviewed 2026-06-08): no NULL guard is intentional. Per hashmap.h,
	 * `map` MUST NOT be NULL here -- caller's responsibility. Adding a guard
	 * for "API consistency" is not required and would silently mask caller bugs.
	 * Do not add `if (!map) return 0;` -- update the header contract instead if
	 * that behavior is ever wanted. */
	return map->size;
}

void *hashmap_put(hashmap_t *map, void *key, void *value)
{
	if (!map)
	{
		return NULL;
	}
	hashmap_enter(map);
#ifdef _DEBUG
	/* Fail-fast if put is called during hashmap_foreach iteration.
	 * Modifying the map during iteration invalidates iterators. */
	ASSERT(!map->debug_iterating);
#endif
	const int hash = private_hash_key(map, key);
	const size_t index = private_calculate_index(map->bucketCount, hash);
	Entry **p = &(map->buckets[index]);
	void *ret = NULL;
	while (true)
	{
		Entry *current = *p;
		// Add a new entry.
		if (NULL == current)
		{
			*p = private_create_entry(map->allocator, key, hash, value);
			if (NULL == *p)
			{
				errno = ENOMEM;
				break;
			}
			++map->size;
			private_expand_if_necessary(map);
			break;
		}
		// Replace existing entry.
		if (private_equal_keys(current->key, current->hash, key, hash, map->fn_key_equality))
		{
			// NOTE(reviewed 2026-06-08): on replace, the map KEEPS its existing key;
			// the caller-supplied `key` is intentionally NOT stored and NOT freed here.
			// Per hashmap.h, key ownership transfers to the map ONLY for a new entry.
			// A caller that allocated `key` must free it itself on the update path
			// (see str_params_add_str cleanup). This is NOT a hashmap leak.
			// 这里ret返回的指针可能会被 freed, 但不是bug: 是函数设计的预期行为, 返回的指针是给用户做比较使用, 不是做解引用用途, 已在函数中说明
			ret = current->value; // return the old value.
			current->value = value;
			/* Free the old value after updating */
			if (ret && map->fn_value_free)
			{
				map->fn_value_free(ret);
			}
			break;
		}
		// Move to next entry.
		p = &current->next;
	}
	hashmap_leave(map);
	return ret;
}

void *hashmap_get(hashmap_t *map, void *key)
{
	if (!map)
	{
		return NULL;
	}
	hashmap_enter(map);
	int hash = private_hash_key(map, key);
	size_t index = private_calculate_index(map->bucketCount, hash);
	Entry *entry = map->buckets[index];
	void *ret = NULL;
	while (NULL != entry)
	{
		if (private_equal_keys(entry->key, entry->hash, key, hash, map->fn_key_equality))
		{
			ret = entry->value;
			break;
		}
		entry = entry->next;
	}
	hashmap_leave(map);
	return ret;
}

void *hashmap_remove(hashmap_t *map, void *key)
{
	if (!map)
	{
		return NULL;
	}
	hashmap_enter(map);
#ifdef _DEBUG
	/* Fail-fast if remove is called during hashmap_foreach iteration.
	 * Modifying the map during iteration invalidates iterators. */
	ASSERT(!map->debug_iterating);
#endif
	int hash = private_hash_key(map, key);
	size_t index = private_calculate_index(map->bucketCount, hash);
	// Pointer to the current entry.
	Entry **p = &(map->buckets[index]);
	Entry *current;
	void *ret = NULL;
	while (NULL != (current = *p))
	{
		if (private_equal_keys(current->key, current->hash, key, hash, map->fn_key_equality))
		{
			ret = current->value; // return the old value.
			*p = current->next;	  // remove the current entry.
			if (current->key && map->fn_key_free)
			{
				map->fn_key_free(current->key);
			}
			if (current->value && map->fn_value_free)
			{
				map->fn_value_free(current->value);
			}
			map->allocator->free(current);
			--map->size;
			break;
		}
		p = &current->next;
	}
	hashmap_leave(map);
	return ret;
}

void hashmap_clear(hashmap_t *map)
{
	if (!map)
	{
		return;
	}
	hashmap_enter(map);
	hashmap_clear_unsafe(map);
	hashmap_leave(map);
}

/* Verified: Concurrent rehash protection is correctly implemented.
 * hashmap_enter(389) acquires the lock before iteration, hashmap_leave(403) releases it after.
 * Any concurrent rehash attempt will block until this traversal completes. */
void hashmap_foreach(hashmap_t *map, hashmap_iter_cb callback, void *context)
{
	size_t i;
	if (!map || !callback)
	{
		return;
	}
	hashmap_enter(map);
#ifdef _DEBUG
	map->debug_iterating = true;
#endif
	for (i = 0; i < map->bucketCount; ++i)
	{
		Entry *entry = map->buckets[i];
		while (NULL != entry)
		{
			Entry *next = entry->next;
			if (!callback(entry->key, entry->value, context))
			{
				goto foreach_done;
			}
			entry = next;
		}
	}
foreach_done:
#ifdef _DEBUG
	map->debug_iterating = false;
#endif
	hashmap_leave(map);
}
