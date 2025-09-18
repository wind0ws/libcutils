#pragma once
#ifndef LCU_LIST_H
#define LCU_LIST_H
/**
 * @file list.h
 * @brief Linked list implementation
 * @reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/list.h
 */

#include <stdbool.h> /* for true/false */
#include <stddef.h>  /* for size_t     */

#ifndef UNUSED
#define UNUSED(x)           (void)(x)
#endif // !UNUSED
#ifndef UNUSED_ATTR
#ifdef _WIN32
#define UNUSED_ATTR 
#else
#define UNUSED_ATTR         __attribute__((unused))
#endif // _WIN32
#endif // !UNUSED_ATTR

#ifdef __cplusplus
extern "C" {
#endif

struct list_node_t;
typedef struct list_node_t list_node_t;
struct list_t;
typedef struct list_t list_t;

/**
 * @brief Callback function prototype for freeing list data
 * @param data Pointer to the data to be freed
 */
typedef void (*list_free_cb)(void *data);

/**
 * @brief Iterator callback prototype used for list_foreach
 * @param data Pointer to the list item currently being iterated
 * @param context User-defined value passed into list_foreach
 * @return true to continue iterating, false to stop iterating
 */
typedef bool (*list_iter_cb)(void *data, void *context);

/**
 * @brief Creates a new, empty list
 * 
 * @param callback Function to be called whenever a list element is removed from the list.
 *                Can be used to release resources held by the list element, e.g.
 *                memory or file descriptor. May be NULL if no cleanup is necessary.
 * @return list_t* Pointer to the new list, or NULL if memory allocation failed
 * @note The returned list must be freed with list_free()
 */
list_t *list_new(list_free_cb callback);

/**
 * @brief Frees the list
 * 
 * @param list Pointer to the list to be freed
 * @note This function accepts NULL as an argument, in which case it behaves like a no-op.
 */
void list_free(list_t *list);

/**
 * @brief Checks if the list is empty
 * 
 * @param list Pointer to the list (must not be NULL)
 * @return true if list is empty (has no elements), false otherwise
 */
bool list_is_empty(const list_t *list);

/**
 * @brief Checks if the list contains specific data
 * 
 * @param list Pointer to the list (must not be NULL)
 * @param data Pointer to the data to search for
 * @return true if the list contains the data, false otherwise
 */
bool list_contains(const list_t *list, const void *data);

/**
 * @brief Gets the length of the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @return size_t Number of elements in the list
 */
size_t list_length(const list_t *list);

/**
 * @brief Gets the first element in the list without removing it
 * 
 * @param list Pointer to the list (must not be NULL or empty)
 * @return void* Pointer to the first element
 */
void *list_front(const list_t *list);

/**
 * @brief Gets the last element in the list without removing it
 * 
 * @param list Pointer to the list (must not be NULL or empty)
 * @return void* Pointer to the last element
 */
void *list_back(const list_t *list);

/**
 * @brief Gets the last node in the list without removing it
 * 
 * @param list Pointer to the list (must not be NULL or empty)
 * @return list_node_t* Pointer to the last node
 */
list_node_t *list_back_node(const list_t *list);

/**
 * @brief Inserts data after a specific node in the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @param prev_node Pointer to the previous node (must not be NULL)
 * @param data Pointer to the data to insert (must not be NULL)
 * @return true if data was successfully inserted, false otherwise (e.g. out of memory)
 * @note This function does not make a copy of data, so the pointer must remain valid
 *       at least until the element is removed from the list or the list is freed.
 */
bool list_insert_after(list_t *list, list_node_t *prev_node, void *data);

/**
 * @brief Inserts data at the beginning of the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @param data Pointer to the data to insert (must not be NULL)
 * @return true if data was successfully inserted, false otherwise (e.g. out of memory)
 * @note This function does not make a copy of data, so the pointer must remain valid
 *       at least until the element is removed from the list or the list is freed.
 */
bool list_prepend(list_t *list, void *data);

/**
 * @brief Inserts data at the end of the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @param data Pointer to the data to insert (must not be NULL)
 * @return true if data was successfully inserted, false otherwise (e.g. out of memory)
 * @note This function does not make a copy of data, so the pointer must remain valid
 *       at least until the element is removed from the list or the list is freed.
 */
bool list_append(list_t *list, void *data);

/**
 * @brief Removes specific data from the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @param data Pointer to the data to remove (must not be NULL)
 * @return true if data was found and removed, false otherwise
 * @note If data is inserted multiple times in the list, this function will only remove
 *       the first instance. If a free function was specified in list_new, it will be
 *       called with the data.
 */
bool list_remove(list_t *list, void *data);

/**
 * @brief Removes all elements from the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @note Calling this function will return the list to the same state it was in after list_new.
 */
void list_clear(list_t *list);

/**
 * @brief Iterates through the list and calls callback for each data element
 * 
 * @param list Pointer to the list (must not be NULL)
 * @param callback Function to call for each element (must not be NULL)
 * @param context User-defined data passed to callback on each iteration
 * @return list_node_t* Pointer to the last processed element, or NULL if the list is empty
 *                      or all calls to callback returned true
 * @note Iteration continues until callback returns false. If the list is empty, callback
 *       will never be called. It is safe to mutate the list inside the callback. If an
 *       element is added before the node being visited, there will be no callback for the
 *       newly-inserted node.
 */
list_node_t *list_foreach(const list_t *list, list_iter_cb callback, void *context);

/**
 * @brief Gets an iterator to the first element in the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @return list_node_t* Iterator to the first element
 * @note The returned iterator is valid as long as it does not equal the value returned
 *       by list_end().
 */
list_node_t *list_begin(const list_t *list);

/**
 * @brief Gets an iterator that points past the end of the list
 * 
 * @param list Pointer to the list (must not be NULL)
 * @return list_node_t* Iterator pointing past the end of the list
 * @note This function returns the value of an invalid iterator for the given list.
 *       When an iterator has the same value as what's returned by this function, you
 *       may no longer call list_next() with the iterator.
 */
list_node_t* list_end(UNUSED_ATTR const list_t* list);

/**
 * @brief Gets the next value for a valid iterator
 * 
 * @param node Pointer to the current iterator node
 * @return list_node_t* Next iterator value
 * @note If the returned value equals the value returned by list_end(), the iterator
 *       has reached the end of the list and may no longer be used for any purpose.
 */
list_node_t *list_next(const list_node_t *node);

/**
 * @brief Gets the value stored at the location pointed to by the iterator
 * 
 * @param node Pointer to the iterator node
 * @return void* Pointer to the stored value
 * @note node must not equal the value returned by list_end().
 */
void *list_node(const list_node_t *node);

#ifdef __cplusplus
}
#endif

#endif // !LCU_LIST_H
