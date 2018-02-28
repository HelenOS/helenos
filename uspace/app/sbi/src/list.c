/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @file Doubly linked list.
 *
 * Circular, with a head. Nodes structures are allocated separately from data.
 * Data is stored as 'void *'. We implement several sanity checks to prevent
 * common usage errors.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mytypes.h"

#include "list.h"

static list_node_t *list_node_new(void *data);
static void list_node_delete(list_node_t *node);
static void list_node_insert_between(list_node_t *n, list_node_t *a, list_node_t *b);
static void list_node_unlink(list_node_t *n);
static bool_t list_node_present(list_t *list, list_node_t *node);

/** Initialize list.
 *
 * @param list	List to initialize.
 */
void list_init(list_t *list)
{
	list->head.prev = &list->head;
	list->head.next = &list->head;
}

/** Deinitialize list.
 *
 * @param list	List to deinitialize.
 */
void list_fini(list_t *list)
{
	assert(list_is_empty(list));

	list->head.prev = NULL;
	list->head.next = NULL;
}

/** Append data to list.
 *
 * Create a new list node and append it at the end of the list.
 *
 * @param list	Linked list.
 * @param data	Data for the new node.
 */
void list_append(list_t *list, void *data)
{
	list_node_t *node;

	node = list_node_new(data);
	list_node_insert_between(node, list->head.prev, &list->head);
}

/** Prepend data to list.
 *
 * Create a new list node and prepend it at the beginning of the list.
 *
 * @param list	Linked list.
 * @param data	Data for the new node.
 */
void list_prepend(list_t *list, void *data)
{
	list_node_t *node;

	node = list_node_new(data);
	list_node_insert_between(node, list->head.prev, &list->head);
}

/** Remove data from list.
 *
 * Removes the given node from a list and destroys it. Any data the node might
 * have is ignored. If asserts are on, we check wheter node is really present
 * in the list the caller is requesting us to remove it from.
 *
 * @param list	Linked list.
 * @param node	List node to remove.
 */
void list_remove(list_t *list, list_node_t *node)
{
	/* Check whether node is in the list as claimed. */
	assert(list_node_present(list, node));
	list_node_unlink(node);
	node->data = NULL;
	list_node_delete(node);
}

/** Return first list node or NULL if list is empty.
 *
 * @param list	Linked list.
 * @return	First node of the list or @c NULL if the list is empty.
 */
list_node_t *list_first(list_t *list)
{
	list_node_t *node;

	assert(list != NULL);
	node = list->head.next;

	return (node != &list->head) ? node : NULL;
}

/** Return last list node or NULL if list is empty.
 *
 * @param list	Linked list.
 * @return	Last node of the list or @c NULL if the list is empty.
 */
list_node_t *list_last(list_t *list)
{
	list_node_t *node;

	assert(list != NULL);
	node = list->head.prev;

	return (node != &list->head) ? node : NULL;
}

/** Return next list node or NULL if this was the last one.
 *
 * @param list	Linked list.
 * @param node	Node whose successor we are interested in.
 * @return	Following list node or @c NULL if @a node is last.
 */
list_node_t *list_next(list_t *list, list_node_t *node)
{
	(void) list;
	assert(list != NULL);
	assert(node != NULL);

	return (node->next != &list->head) ? node->next : NULL;
}

/** Return previous list node or NULL if this was the last one.
 *
 * @param list	Linked list.
 * @param node	Node whose predecessor we are interested in.
 * @return	Preceding list node or @c NULL if @a node is last.
 */
list_node_t *list_prev(list_t *list, list_node_t *node)
{
	(void) list;
	assert(list != NULL);
	assert(node != NULL);

	return (node->prev != &list->head) ? node->prev : NULL;
}

/** Return b_true if list is empty.
 *
 * @param list	Linked list.
 * @return	@c b_true if list is empty, @c b_false otherwise.
 */
bool_t list_is_empty(list_t *list)
{
	return (list_first(list) == NULL);
}

/** Change node data.
 *
 * Change the data associated with a node.
 *
 * @param node	List node.
 * @param data	New data for node.
 */
void list_node_setdata(list_node_t *node, void *data)
{
	node->data = data;
}

/** Create new node.
 *
 * @param data	Initial data for node.
 * @return	New list node.
 */
static list_node_t *list_node_new(void *data)
{
	list_node_t *node;

	node = malloc(sizeof(list_node_t));
	if (node == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	node->prev = NULL;
	node->next = NULL;
	node->data = data;

	return node;
}

/** Delete node.
 *
 * @param node	List node. Must not take part in any list.
 */
static void list_node_delete(list_node_t *node)
{
	assert(node->prev == NULL);
	assert(node->next == NULL);
	assert(node->data == NULL);
	free(node);
}

/** Insert node between two other nodes.
 *
 * Inserts @a n between neighboring nodes @a a and @a b.
 *
 * @param n	Node to insert.
 * @param a	Node to precede @a n.
 * @param b	Node to follow @a n.
 */
static void list_node_insert_between(list_node_t *n, list_node_t *a,
    list_node_t *b)
{
	assert(n->prev == NULL);
	assert(n->next == NULL);
	n->prev = a;
	n->next = b;

	assert(a->next == b);
	assert(b->prev == a);
	a->next = n;
	b->prev = n;
}

/** Unlink node.
 *
 * Unlink node from the list it is currently in.
 *
 * @param n	Node to unlink from its current list.
 */
static void list_node_unlink(list_node_t *n)
{
	list_node_t *a, *b;
	assert(n->prev != NULL);
	assert(n->next != NULL);

	a = n->prev;
	b = n->next;

	assert(a->next == n);
	assert(b->prev == n);

	a->next = b;
	b->prev = a;

	n->prev = NULL;
	n->next = NULL;
}

/** Check whether @a node is in list @a list.
 *
 * @param list	Linked list.
 * @param node	Node.
 * @return	@c b_true if @a node is part of @a list, @c b_false otherwise.
 */
static bool_t list_node_present(list_t *list, list_node_t *node)
{
	list_node_t *m;

	m = list->head.next;
	while (m != &list->head) {
		if (m == node)
			return b_true;
		m = m->next;
	}

	return b_false;
}
