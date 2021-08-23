/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LIST_H__
#define __LIST_H__

#include "include/list.h"

static inline bool list_empty(list_t *list)
{
	return (list->next == list);
}

static inline bool entry_linked(list_entry_t *entry)
{
	return entry->next != entry;
}

/* An entry is initialized when it points to itself */
static inline void list_init_entry(list_entry_t *entry)
{
	entry->next = entry;
	entry->prev = entry;
}

/* An initialized list is just an initialized entry */
static inline void list_init(list_t *list)
{
	list_init_entry(list);
}

/* Link an entry between two other entries in the list */
static inline void list_link_entry(list_entry_t *entry, list_entry_t *prev,
		list_entry_t *next)
{
	entry->next = next;
	next->prev = entry;
	entry->prev = prev;
	prev->next = entry;
}

/* Unlink an entry from the list */
static inline void list_unlink_entry(list_entry_t *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	list_init_entry(entry);
}

/* Adding an entry in the head of a list is equivalent to linking it
 * between the head of the list and its next */
static inline void list_add_head(list_t *list, list_entry_t *entry)
{
	list_link_entry(entry, list, list->next);
}

/* Adding an entry in the tail of a list is equivalent to linking it
 * between the head of the list and its prev */
static inline void list_add_tail(list_t *list, list_entry_t *entry)
{
	list_link_entry(entry, list->prev, list);
}

static inline list_entry_t *list_remove_head(list_t *list)
{
	if (list_empty(list))
		return NULL;

	list_entry_t *ret = list->next;
	list_unlink_entry(ret);

	return ret;
}

static inline list_entry_t *list_remove_tail(list_t *list)
{
	if (list_empty(list))
		return NULL;

	list_entry_t *ret = list->prev;
	list_unlink_entry(ret);

	return ret;
}

/* Get the container of an entry
 *
 * This is a helper macro that allows us to get a pointer to
 * the struct that contains a list entry
 *
 * \param[in] ptr A pointer to the entry
 * \param[in] type The type of the container
 * \param[in] name The name of the container entry field
 *
 * \returns A pointer to the container including the entry
 *
 */
#define get_container(ptr, type, name) \
	(type *)((char *)ptr - offsetof(type, name))

/* Iterate through all the entries of a list
 *
 * \param[out] iter An iterator to use for keeping the pointer to
 *             the current list entry of each iteration
 * \param[in] list The list head of the list to iterate
 */
#define for_each(iter, list) \
	for (iter = (list)->next; iter != (list); iter = iter->next)

/* Iterate through all the containers of a list
 *
 * \param[out] iter An iterator to use for keeping the pointer to
 *             the current list entry container for each iteration
 * \param[in] list The head of the list to iterate
 * \param[in] member The name of list_entry_t field of the container
 */
#define for_each_container(iter, list, type, member) \
	for (iter = get_container((list)->next, type, member); \
		&iter->member != (list); \
	       iter = get_container(iter->member.next, type, member))

/* Iterate through all the containers of a list safely
 *
 * This flavour of the iterator allows you to modify the container you
 * are accessing in each iteration, e.g. remove it from the list.
 *
 * \param[out] iter An iterator to use for keeping the pointer to
 *             the current list entry container for each iteration
 * \param[out] tmp_iter An iterator to use temporarily for safely
 *             traversing the list
 * \param[in] list The head of the list to iterate
 * \param[in] member The name of list_entry_t field of the container
 */
#define for_each_container_safe(iter, tmp_iter, list, type, member) \
	for (iter = get_container((list)->next, type, member), \
		tmp_iter = get_container(iter->member.next, type, member); \
		&iter->member != (list); \
		iter = tmp_iter, \
		tmp_iter = get_container(tmp_iter->member.next, type, member))


#endif /* __LIST_H__ */
