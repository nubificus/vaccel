// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/list.h" // IWYU pragma: export
#include <stddef.h>

/* Check if list is empty */
static inline bool list_empty(struct vaccel_list_entry *list)
{
	return (list->next == list);
}

/* Check if list entry is linked */
static inline bool list_entry_linked(struct vaccel_list_entry *entry)
{
	return entry->next != entry;
}

/* Initialize list entry.
 *
 * An entry is initialized when it points to itself. */
static inline void list_init_entry(struct vaccel_list_entry *entry)
{
	entry->next = entry;
	entry->prev = entry;
}

/* Initialize list.
 *
 * An initialized list is just an initialized entry. */
static inline void list_init(struct vaccel_list_entry *list)
{
	list_init_entry(list);
}

/* Link entry between two other entries in the list */
static inline void list_link_entry(struct vaccel_list_entry *entry,
				   struct vaccel_list_entry *prev,
				   struct vaccel_list_entry *next)
{
	entry->next = next;
	next->prev = entry;
	entry->prev = prev;
	prev->next = entry;
}

/* Unlink entry from its list */
static inline void list_unlink_entry(struct vaccel_list_entry *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	list_init_entry(entry);
}

/* Add entry at the head of the list.
 *
 * Adding an entry at the head of a list is equivalent to linking it
 * between the head of the list and its next. */
static inline void list_add_head(struct vaccel_list_entry *list,
				 struct vaccel_list_entry *entry)
{
	list_link_entry(entry, list, list->next);
}

/* Add entry at the tail of the list.
 *
 * Adding an entry in the tail of a list is equivalent to linking it
 * between the head of the list and its prev. */
static inline void list_add_tail(struct vaccel_list_entry *list,
				 struct vaccel_list_entry *entry)
{
	list_link_entry(entry, list->prev, list);
}

/* Remove the entry at the head of the list */
static inline struct vaccel_list_entry *
list_remove_head(struct vaccel_list_entry *list)
{
	if (list_empty(list))
		return NULL;

	struct vaccel_list_entry *ret = list->next;
	list_unlink_entry(ret);

	return ret;
}

/* Remove the entry at the tail of the list */
static inline struct vaccel_list_entry *
list_remove_tail(struct vaccel_list_entry *list)
{
	if (list_empty(list))
		return NULL;

	struct vaccel_list_entry *ret = list->prev;
	list_unlink_entry(ret);

	return ret;
}

/* Get the container of an entry.
 *
 * This is a helper macro that allows us to get a pointer to
 * the struct that contains a list entry.
 *
 * \param[in] ptr A pointer to the entry
 * \param[in] type The type of the container
 * \param[in] name The name of the container entry field
 *
 * \returns A pointer to the container including the entry
 *
 */
#define list_get_container(ptr, type, name) \
	(type *)((char *)(ptr) - offsetof(type, name))

/* Iterate through all the entries of a list.
 *
 * \param[out] iter An iterator to use for keeping the pointer to
 *             the current list entry of each iteration
 * \param[in] list The list head of the list to iterate
 */
#define list_for_each(iter, list) \
	for ((iter) = (list)->next; (iter) != (list); (iter) = (iter)->next)

/* Iterate through all the containers of a list.
 *
 * \param[out] iter An iterator to use for keeping the pointer to
 *             the current list entry container for each iteration
 * \param[in] list The head of the list to iterate
 * \param[in] type The type of the container
 * \param[in] member The name of struct vaccel_list_entry field of the container
 */
#define list_for_each_container(iter, list, type, member)             \
	for ((iter) = list_get_container((list)->next, type, member); \
	     &(iter)->member != (list);                               \
	     (iter) = list_get_container((iter)->member.next, type, member))

/* Iterate through all the containers of a list safely.
 *
 * Allows modification of the accessed container in each iteration, e.g. removal
 * from the list.
 *
 * \param[out] iter An iterator to use for keeping the pointer to
 *             the current list entry container for each iteration
 * \param[out] tmp_iter An iterator to use temporarily for safely
 *             traversing the list
 * \param[in] list The head of the list to iterate
 * \param[in] type The type of the container
 * \param[in] member The name of struct vaccel_list_entry field of the container
 */
#define list_for_each_container_safe(iter, tmp_iter, list, type, member)    \
	for ((iter) = list_get_container((list)->next, type, member),       \
	    (tmp_iter) =                                                    \
		     list_get_container((iter)->member.next, type, member); \
	     &(iter)->member != (list); (iter) = (tmp_iter),                \
	    (tmp_iter) = list_get_container((tmp_iter)->member.next, type,  \
					    member))
