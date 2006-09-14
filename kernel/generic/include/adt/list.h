/*
 * Copyright (C) 2001-2004 Jakub Jermar
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
/** @file
 */

#ifndef KERN_LIST_H_
#define KERN_LIST_H_

#include <arch/types.h>
#include <typedefs.h>

/** Doubly linked list head and link type. */
struct link {
	link_t *prev;	/**< Pointer to the previous item in the list. */
	link_t *next;	/**< Pointer to the next item in the list. */
};

/** Declare and initialize statically allocated list.
 *
 * @param name Name of the new statically allocated list.
 */
#define LIST_INITIALIZE(name)		link_t name = { .prev = &name, .next = &name }

/** Initialize doubly-linked circular list link
 *
 * Initialize doubly-linked list link.
 *
 * @param link Pointer to link_t structure to be initialized.
 */
static inline void link_initialize(link_t *link)
{
	link->prev = NULL;
	link->next = NULL;
}

/** Initialize doubly-linked circular list
 *
 * Initialize doubly-linked circular list.
 *
 * @param head Pointer to link_t structure representing head of the list.
 */
static inline void list_initialize(link_t *head)
{
	head->prev = head;
	head->next = head;
}

/** Add item to the beginning of doubly-linked circular list
 *
 * Add item to the beginning of doubly-linked circular list.
 *
 * @param link Pointer to link_t structure to be added.
 * @param head Pointer to link_t structure representing head of the list.
 */
static inline void list_prepend(link_t *link, link_t *head)
{
	link->next = head->next;
	link->prev = head;
	head->next->prev = link;
	head->next = link;
}

/** Add item to the end of doubly-linked circular list
 *
 * Add item to the end of doubly-linked circular list.
 *
 * @param link Pointer to link_t structure to be added.
 * @param head Pointer to link_t structure representing head of the list.
 */
static inline void list_append(link_t *link, link_t *head)
{
	link->prev = head->prev;
	link->next = head;
	head->prev->next = link;
	head->prev = link;
}

/** Remove item from doubly-linked circular list
 *
 * Remove item from doubly-linked circular list.
 *
 * @param link Pointer to link_t structure to be removed from the list it is contained in.
 */
static inline void list_remove(link_t *link)
{
	link->next->prev = link->prev;
	link->prev->next = link->next;
	link_initialize(link);
}

/** Query emptiness of doubly-linked circular list
 *
 * Query emptiness of doubly-linked circular list.
 *
 * @param head Pointer to link_t structure representing head of the list.
 */
static inline bool list_empty(link_t *head)
{
	return head->next == head ? true : false;
}


/** Split or concatenate headless doubly-linked circular list
 *
 * Split or concatenate headless doubly-linked circular list.
 *
 * Note that the algorithm works both directions:
 * concatenates splitted lists and splits concatenated lists.
 *
 * @param part1 Pointer to link_t structure leading the first (half of the headless) list.
 * @param part2 Pointer to link_t structure leading the second (half of the headless) list. 
 */
static inline void headless_list_split_or_concat(link_t *part1, link_t *part2)
{
	link_t *hlp;

	part1->prev->next = part2;
	part2->prev->next = part1;	
	hlp = part1->prev;
	part1->prev = part2->prev;
	part2->prev = hlp;
}


/** Split headless doubly-linked circular list
 *
 * Split headless doubly-linked circular list.
 *
 * @param part1 Pointer to link_t structure leading the first half of the headless list.
 * @param part2 Pointer to link_t structure leading the second half of the headless list. 
 */
static inline void headless_list_split(link_t *part1, link_t *part2)
{
	headless_list_split_or_concat(part1, part2);
}

/** Concatenate two headless doubly-linked circular lists
 *
 * Concatenate two headless doubly-linked circular lists.
 *
 * @param part1 Pointer to link_t structure leading the first headless list.
 * @param part2 Pointer to link_t structure leading the second headless list. 
 */
static inline void headless_list_concat(link_t *part1, link_t *part2)
{
	headless_list_split_or_concat(part1, part2);
}

#define list_get_instance(link,type,member) (type *)(((uint8_t*)(link))-((uint8_t*)&(((type *)NULL)->member)))

extern bool list_member(const link_t *link, const link_t *head);
extern void list_concat(link_t *head1, link_t *head2);

#endif

/** @}
 */
