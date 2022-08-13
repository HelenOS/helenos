/*
 * SPDX-FileCopyrightText: 2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_adt
 * @{
 */

/**
 * @file
 * @brief	Functions completing doubly linked circular list implementation.
 *
 * This file contains some of the functions implementing doubly linked circular lists.
 * However, this ADT is mostly implemented in @ref list.h.
 */

#include <adt/list.h>

/** Check for membership
 *
 * Check whether link is contained in a list.
 * Membership is defined as pointer equivalence.
 *
 * @param link	Item to look for.
 * @param list	List to look in.
 *
 * @return true if link is contained in list, false otherwise.
 *
 */
bool list_member(const link_t *link, const list_t *list)
{
	bool found = false;
	link_t *hlp = list->head.next;

	while (hlp != &list->head) {
		if (hlp == link) {
			found = true;
			break;
		}
		hlp = hlp->next;
	}

	return found;
}

/** Moves items of one list into another after the specified item.
 *
 * Inserts all items of @a list after item at @a pos in another list.
 * Both lists may be empty.
 *
 * @param list Source list to move after pos. Empty afterwards.
 * @param pos Source items will be placed after this item.
 */
void list_splice(list_t *list, link_t *pos)
{
	if (list_empty(list))
		return;

	/* Attach list to destination. */
	list->head.next->prev = pos;
	list->head.prev->next = pos->next;

	/* Link destination list to the added list. */
	pos->next->prev = list->head.prev;
	pos->next = list->head.next;

	list_initialize(list);
}

/** Count list items
 *
 * Return the number of items in the list.
 *
 * @param list		List to count.
 * @return		Number of items in the list.
 */
unsigned long list_count(const list_t *list)
{
	unsigned long count = 0;

	link_t *link = list_first(list);
	while (link != NULL) {
		count++;
		link = list_next(link, list);
	}

	return count;
}

/** @}
 */
