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
#include "data/hash_map.h"
#include "common_macro.h"
#include <string.h>
#include <errno.h>

typedef struct Entry Entry;
struct Entry {
	void* key;
	int hash;
	void* value;
	Entry* next;
};
struct Hashmap {
	Entry** buckets;
	size_t bucketCount;
	hash_key_fn fn_hash;
	key_free_fn fn_key_free;
	value_free_fn fn_value_free;
	key_equality_fn fn_key_equality;
	hashmap_lock_t lock;
	size_t size;
};

#define hashmap_enter(handle)    if((handle != NULL) &&        \
        ((handle)->lock.acquire != NULL))                    \
        { (handle)->lock.acquire((handle)->lock.arg); }
#define hashmap_leave(handle)    if((handle != NULL) &&        \
        ((handle)->lock.release != NULL))                    \
        { (handle)->lock.release((handle)->lock.arg); }

hashmap_t* hashmap_create(size_t initial_capacity,
	hash_key_fn fn_hash,
	key_free_fn fn_key_free,
	value_free_fn fn_value_free,
	key_equality_fn fn_key_equality,
	hashmap_lock_t* lock) 
{
	hashmap_t* map = (hashmap_t*)(malloc(sizeof(hashmap_t)));
	if (map == NULL) 
	{
		return NULL;
	}
	/* Initialize the hashmap object */
	memset(map, 0, sizeof(*map));
	/* Copy the lock if it is not NULL */
	if (lock != NULL)
	{
		memcpy(&map->lock, lock, sizeof(map->lock));
	}
	// 0.75 load factor.
	size_t minimumBucketCount = initial_capacity * 4 / 3;
	map->bucketCount = 1;
	while (map->bucketCount <= minimumBucketCount) 
	{
		// Bucket count must be power of 2.
		map->bucketCount <<= 1;
	}
	map->buckets = (Entry**)(calloc(map->bucketCount, sizeof(Entry*)));
	if (map->buckets == NULL) 
	{
		free(map);
		return NULL;
	}
	map->fn_hash = fn_hash;
	map->fn_key_free = fn_key_free;
	map->fn_value_free = fn_value_free;
	map->fn_key_equality = fn_key_equality;
	map->size = 0;
	return map;
}
/**
 * Hashes the given key.
 */
#ifdef __clang__
__attribute__((no_sanitize("integer")))
#endif //__clang__
static inline int hashKey(hashmap_t* map, void* key) 
{
	int h = map->fn_hash(key);
	// We apply this secondary hashing discovered by Doug Lea to defend
	// against bad hashes.
	h += ~(h << 9);
	h ^= (((unsigned int)h) >> 14);
	h += (h << 4);
	h ^= (((unsigned int)h) >> 10);
	return h;
}

static inline size_t calculateIndex(size_t bucketCount, int hash) 
{
	return ((size_t)hash) & (bucketCount - 1);
}

static void expandIfNecessary(hashmap_t* map) 
{
	// If the load factor exceeds 0.75...
	if (map->size <= (map->bucketCount * 3 / 4)) 
	{
		return;
	}
	// Start off with a 0.33 load factor.
	size_t newBucketCount = map->bucketCount << 1;
	Entry** newBuckets = (Entry**)(calloc(newBucketCount, sizeof(Entry*)));
	if (newBuckets == NULL)
	{
		// Abort expansion.
		return;
	}
	// Move over existing entries.
	size_t i;
	for (i = 0; i < map->bucketCount; i++)
	{
		Entry* entry = map->buckets[i];
		while (entry != NULL)
		{
			Entry* next = entry->next;
			size_t index = calculateIndex(newBucketCount, entry->hash);
			entry->next = newBuckets[index];
			newBuckets[index] = entry;
			entry = next;
		}
	}
	// Copy over internals.
	free(map->buckets);
	map->buckets = newBuckets;
	map->bucketCount = newBucketCount;
}

static void hashmap_clear_unsafe(hashmap_t* map)
{
	size_t i;
	for (i = 0; i < map->bucketCount; i++) 
	{
		Entry* entry = map->buckets[i];
		while (entry != NULL) 
		{
			Entry* next = entry->next;
			if (map->fn_key_free)
			{
				map->fn_key_free(entry->key);
			}
			if (map->fn_value_free)
			{
				map->fn_value_free(entry->value);
			}
			free(entry);
			map->size--;
			entry = next;
		}
		map->buckets[i] = NULL;
	}
}

void hashmap_free(hashmap_t* map) 
{
	if (!map)
	{
		return;
	}
	hashmap_enter(map);
	hashmap_clear_unsafe(map);
	free(map->buckets);
	map->buckets = NULL;
	map->bucketCount = 0;
	hashmap_leave(map);
	free(map);
}

#ifdef __clang__
__attribute__((no_sanitize("integer")))
#endif
/* FIXME: relies on signed integer overflow, which is undefined behavior */
int hashmap_hash(void* key, size_t keySize)
{
	int h = keySize;
	char* data = (char*)key;
	size_t i;
	for (i = 0; i < keySize; i++) 
	{
		h = h * 31 + *data;
		data++;
	}
	return h;
}

static Entry* createEntry(void* key, int hash, void* value) 
{
	Entry* entry = (Entry*)(malloc(sizeof(Entry)));
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

static inline bool equalKeys(void* keyA, int hashA, void* keyB, int hashB,
	key_equality_fn fn_equals) 
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

size_t hashmap_size(hashmap_t* map)
{
	return map->size;
}

void* hashmap_put(hashmap_t* map, void* key, void* value) 
{
	if (!map)
	{
		return NULL;
	}
	hashmap_enter(map);
	int hash = hashKey(map, key);
	size_t index = calculateIndex(map->bucketCount, hash);
	Entry** p = &(map->buckets[index]);
	void* ret = NULL;
	while (true) 
	{
		Entry* current = *p;
		// Add a new entry.
		if (current == NULL) 
		{
			*p = createEntry(key, hash, value);
			if (*p == NULL) 
			{
				errno = ENOMEM;
				break;
			}
			map->size++;
			expandIfNecessary(map);
			break;
		}
		// Replace existing entry.
		if (equalKeys(current->key, current->hash, key, hash, map->fn_key_equality)) 
		{
			ret = current->value;
			current->value = value;
			break;
		}
		// Move to next entry.
		p = &current->next;
	}
	hashmap_leave(map);
	return ret;
}

void* hashmap_get(hashmap_t* map, void* key) 
{
	if (!map)
	{
		return NULL;
	}
	hashmap_enter(map);
	int hash = hashKey(map, key);
	size_t index = calculateIndex(map->bucketCount, hash);
	Entry* entry = map->buckets[index];
	void* ret = NULL;
	while (entry != NULL) 
	{
		if (equalKeys(entry->key, entry->hash, key, hash, map->fn_key_equality)) 
		{
			ret = entry->value;
			break;
		}
		entry = entry->next;
	}
	hashmap_leave(map);
	return ret;
}

void* hashmap_remove(hashmap_t* map, void* key) 
{
	if (!map)
	{
		return NULL;
	}
	hashmap_enter(map);
	int hash = hashKey(map, key);
	size_t index = calculateIndex(map->bucketCount, hash);
	// Pointer to the current entry.
	Entry** p = &(map->buckets[index]);
	Entry* current;
	void* ret = NULL;
	while ((current = *p) != NULL) 
	{
		if (equalKeys(current->key, current->hash, key, hash, map->fn_key_equality)) 
		{
			ret = current->value;
			*p = current->next;
			if (map->fn_key_free)
			{
				map->fn_key_free(current->key);
			}
			if (map->fn_value_free)
			{
				map->fn_value_free(current->value);
			}
			free(current);
			map->size--;
			break;
		}
		p = &current->next;
	}
	hashmap_leave(map);
	return ret;
}

void hashmap_clear(hashmap_t* map)
{
	if (!map)
	{
		return;
	}
	hashmap_enter(map);
	hashmap_clear_unsafe(map);
	hashmap_leave(map);
}

void hashmap_foreach(hashmap_t* map, hashmap_iter_cb callback, void* context) {
	size_t i;
	if (!map)
	{
		return;
	}
	hashmap_enter(map);
	for (i = 0; i < map->bucketCount; i++) 
	{
		Entry* entry = map->buckets[i];
		while (entry != NULL) 
		{
			Entry* next = entry->next;
			if (!callback(entry->key, entry->value, context)) 
			{
				break;
			}
			entry = next;
		}
	}
	hashmap_leave(map);
}