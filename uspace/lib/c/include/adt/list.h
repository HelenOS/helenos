/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2013 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_LIST_H_
#define LIBC_LIST_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <trace.h>

/** Doubly linked list link. */
typedef struct link {
	struct link *prev;  /**< Pointer to the previous item in the list. */
	struct link *next;  /**< Pointer to the next item in the list. */
} link_t;

/** Doubly linked list. */
typedef struct list {
	link_t head;  /**< List head. Does not have any data. */
} list_t;

extern bool list_member(const link_t *, const list_t *);
extern void list_splice(list_t *, link_t *);
extern unsigned long list_count(const list_t *);

/** Declare and initialize statically allocated list.
 *
 * @param name Name of the new statically allocated list.
 *
 */
#define LIST_INITIALIZE(name) \
	list_t name = LIST_INITIALIZER(name)

/** Initializer for statically allocated list.
 *
 * @code
 * struct named_list {
 *     const char *name;
 *     list_t list;
 * } var = {
 *     .name = "default name",
 *     .list = LIST_INITIALIZER(name_list.list)
 * };
 * @endcode
 *
 * @param name Name of the new statically allocated list.
 *
 */
#define LIST_INITIALIZER(name) \
	{ \
		.head = { \
			.prev = &(name).head, \
			.next = &(name).head \
		} \
	}

#define list_get_instance(link, type, member) \
	((type *) (((void *)(link)) - list_link_to_void(&(((type *) NULL)->member))))

#define list_foreach(list, member, itype, iterator) \
	for (itype *iterator = NULL; iterator == NULL; iterator = (itype *) 1) \
		for (link_t *_link = (list).head.next; \
		    iterator = list_get_instance(_link, itype, member), \
		    _link != &(list).head; _link = _link->next)

#define list_foreach_rev(list, member, itype, iterator) \
	for (itype *iterator = NULL; iterator == NULL; iterator = (itype *) 1) \
		for (link_t *_link = (list).head.prev; \
		    iterator = list_get_instance(_link, itype, member), \
		    _link != &(list).head; _link = _link->prev)

/** Unlike list_foreach(), allows removing items while traversing a list.
 *
 * @code
 * list_t mylist;
 * typedef struct item {
 *     int value;
 *     link_t item_link;
 * } item_t;
 *
 * //..
 *
 * // Print each list element's value and remove the element from the list.
 * list_foreach_safe(mylist, cur_link, next_link) {
 *     item_t *cur_item = list_get_instance(cur_link, item_t, item_link);
 *     printf("%d\n", cur_item->value);
 *     list_remove(cur_link);
 * }
 * @endcode
 *
 * @param list List to traverse.
 * @param iterator Iterator to the current element of the list.
 *             The item this iterator points may be safely removed
 *             from the list.
 * @param next_iter Iterator to the next element of the list.
 */
#define list_foreach_safe(list, iterator, next_iter) \
	for (link_t *iterator = (list).head.next, \
	    *next_iter = iterator->next; \
	    iterator != &(list).head; \
	    iterator = next_iter, next_iter = iterator->next)

#define assert_link_not_used(link) \
	assert(!link_used(link))

/** Returns true if the link is definitely part of a list. False if not sure. */
static inline bool link_in_use(const link_t *link)
{
	return link->prev != NULL && link->next != NULL;
}

/** Initialize doubly-linked circular list link
 *
 * Initialize doubly-linked list link.
 *
 * @param link Pointer to link_t structure to be initialized.
 *
 */
NO_TRACE static inline void link_initialize(link_t *link)
{
	link->prev = NULL;
	link->next = NULL;
}

/** Initialize doubly-linked circular list
 *
 * Initialize doubly-linked circular list.
 *
 * @param list Pointer to list_t structure.
 *
 */
NO_TRACE static inline void list_initialize(list_t *list)
{
	list->head.prev = &list->head;
	list->head.next = &list->head;
}

/** Insert item before another item in doubly-linked circular list.
 *
 */
static inline void list_insert_before(link_t *lnew, link_t *lold)
{
	lnew->next = lold;
	lnew->prev = lold->prev;
	lold->prev->next = lnew;
	lold->prev = lnew;
}

/** Insert item after another item in doubly-linked circular list.
 *
 */
static inline void list_insert_after(link_t *lnew, link_t *lold)
{
	lnew->prev = lold;
	lnew->next = lold->next;
	lold->next->prev = lnew;
	lold->next = lnew;
}

/** Add item to the beginning of doubly-linked circular list
 *
 * Add item to the beginning of doubly-linked circular list.
 *
 * @param link Pointer to link_t structure to be added.
 * @param list Pointer to list_t structure.
 *
 */
NO_TRACE static inline void list_prepend(link_t *link, list_t *list)
{
	list_insert_after(link, &list->head);
}

/** Add item to the end of doubly-linked circular list
 *
 * Add item to the end of doubly-linked circular list.
 *
 * @param link Pointer to link_t structure to be added.
 * @param list Pointer to list_t structure.
 *
 */
NO_TRACE static inline void list_append(link_t *link, list_t *list)
{
	list_insert_before(link, &list->head);
}

/** Remove item from doubly-linked circular list
 *
 * Remove item from doubly-linked circular list.
 *
 * @param link Pointer to link_t structure to be removed from the list
 *             it is contained in.
 *
 */
NO_TRACE static inline void list_remove(link_t *link)
{
	if ((link->prev != NULL) && (link->next != NULL)) {
		link->next->prev = link->prev;
		link->prev->next = link->next;
	}

	link_initialize(link);
}

/** Query emptiness of doubly-linked circular list
 *
 * Query emptiness of doubly-linked circular list.
 *
 * @param list Pointer to lins_t structure.
 *
 */
NO_TRACE static inline bool list_empty(const list_t *list)
{
	return (list->head.next == &list->head);
}

/** Get first item in list.
 *
 * @param list Pointer to list_t structure.
 *
 * @return Head item of the list.
 * @return NULL if the list is empty.
 *
 */
static inline link_t *list_first(const list_t *list)
{
	return ((list->head.next == &list->head) ? NULL : list->head.next);
}

/** Get last item in list.
 *
 * @param list Pointer to list_t structure.
 *
 * @return Head item of the list.
 * @return NULL if the list is empty.
 *
 */
static inline link_t *list_last(const list_t *list)
{
	return (list->head.prev == &list->head) ? NULL : list->head.prev;
}

/** Get next item in list.
 *
 * @param link Current item link
 * @param list List containing @a link
 *
 * @return Next item or NULL if @a link is the last item.
 */
static inline link_t *list_next(const link_t *link, const list_t *list)
{
	return (link->next == &list->head) ? NULL : link->next;
}

/** Get previous item in list.
 *
 * @param link Current item link
 * @param list List containing @a link
 *
 * @return Previous item or NULL if @a link is the first item.
 */
static inline link_t *list_prev(const link_t *link, const list_t *list)
{
	return (link->prev == &list->head) ? NULL : link->prev;
}

/** Split or concatenate headless doubly-linked circular list
 *
 * Split or concatenate headless doubly-linked circular list.
 *
 * Note that the algorithm works both directions:
 * concatenates splitted lists and splits concatenated lists.
 *
 * @param part1 Pointer to link_t structure leading the first
 *              (half of the headless) list.
 * @param part2 Pointer to link_t structure leading the second
 *              (half of the headless) list.
 *
 */
NO_TRACE static inline void headless_list_split_or_concat(link_t *part1, link_t *part2)
{
	part1->prev->next = part2;
	part2->prev->next = part1;

	link_t *hlp = part1->prev;

	part1->prev = part2->prev;
	part2->prev = hlp;
}

/** Split headless doubly-linked circular list
 *
 * Split headless doubly-linked circular list.
 *
 * @param part1 Pointer to link_t structure leading
 *              the first half of the headless list.
 * @param part2 Pointer to link_t structure leading
 *              the second half of the headless list.
 *
 */
NO_TRACE static inline void headless_list_split(link_t *part1, link_t *part2)
{
	headless_list_split_or_concat(part1, part2);
}

/** Concatenate two headless doubly-linked circular lists
 *
 * Concatenate two headless doubly-linked circular lists.
 *
 * @param part1 Pointer to link_t structure leading
 *              the first headless list.
 * @param part2 Pointer to link_t structure leading
 *              the second headless list.
 *
 */
NO_TRACE static inline void headless_list_concat(link_t *part1, link_t *part2)
{
	headless_list_split_or_concat(part1, part2);
}

/** Concatenate two lists
 *
 * Concatenate lists @a list1 and @a list2, producing a single
 * list @a list1 containing items from both (in @a list1, @a list2
 * order) and empty list @a list2.
 *
 * @param list1		First list and concatenated output
 * @param list2 	Second list and empty output.
 *
 */
NO_TRACE static inline void list_concat(list_t *list1, list_t *list2)
{
	list_splice(list2, list1->head.prev);
}

/** Get n-th item in a list.
 *
 * @param list Pointer to link_t structure representing the list.
 * @param n    Item number (indexed from zero).
 *
 * @return n-th item of the list.
 * @return NULL if no n-th item found.
 *
 */
static inline link_t *list_nth(const list_t *list, unsigned long n)
{
	unsigned long cnt = 0;

	link_t *link = list_first(list);
	while (link != NULL) {
		if (cnt == n)
			return link;

		cnt++;
		link = list_next(link, list);
	}

	return NULL;
}

/** Verify that argument type is a pointer to link_t (at compile time).
 *
 * This can be used to check argument type in a macro.
 */
static inline const void *list_link_to_void(const link_t *link)
{
	return link;
}

/** Determine if link is used.
 *
 * @param link Link
 * @return @c true if link is used, @c false if not.
 */
static inline bool link_used(link_t *link)
{
	if (link->prev == NULL && link->next == NULL)
		return false;

	assert(link->prev != NULL && link->next != NULL);
	return true;
}

#endif

/** @}
 */
