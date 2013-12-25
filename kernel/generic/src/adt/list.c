/*
 * Copyright (c) 2004 Jakub Jermar
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

/** @addtogroup genericadt
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
int list_member(const link_t *link, const list_t *list)
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
void list_concat(list_t *list1, list_t *list2)
{
	if (list_empty(list2))
		return;

	list2->head.next->prev = list1->head.prev;
	list2->head.prev->next = &list1->head;
	list1->head.prev->next = list2->head.next;
	list1->head.prev = list2->head.prev;
	list_initialize(list2);
}

/** Count list items
 *
 * Return the number of items in the list.
 *
 * @param list		List to count.
 * @return		Number of items in the list.
 */
unsigned int list_count(const list_t *list)
{
	unsigned int count = 0;
	
	link_t *link = list_first(list);
	while (link != NULL) {
		count++;
		link = list_next(link, list);
	}
	
	return count;
}

/** @}
 */
