/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** Initialize list. */
void list_init(list_t *list)
{
	list->head.prev = &list->head;
	list->head.next = &list->head;
}

/** Append data to list. */
void list_append(list_t *list, void *data)
{
	list_node_t *node;

	node = list_node_new(data);
	list_node_insert_between(node, list->head.prev, &list->head);
}

/** Prepend data to list. */
void list_prepend(list_t *list, void *data)
{
	list_node_t *node;

	node = list_node_new(data);
	list_node_insert_between(node, list->head.prev, &list->head);
}

/** Remove data from list. */
void list_remove(list_t *list, list_node_t *node)
{
	/* Check whether node is in the list as claimed. */
	assert(list_node_present(list, node));
	list_node_unlink(node);
	node->data = NULL;
	list_node_delete(node);
}

/** Return first list node or NULL if list is empty. */
list_node_t *list_first(list_t *list)
{
	list_node_t *node;

	assert(list != NULL);
	node = list->head.next;

	return (node != &list->head) ? node : NULL;
}

/** Return last list node or NULL if list is empty. */
list_node_t *list_last(list_t *list)
{
	list_node_t *node;

	assert(list != NULL);
	node = list->head.prev;

	return (node != &list->head) ? node : NULL;
}

/** Return next list node or NULL if this was the last one. */
list_node_t *list_next(list_t *list, list_node_t *node)
{
	(void) list;
	assert(list != NULL);
	assert(node != NULL);

	return (node->next != &list->head) ? node->next : NULL;
}

/** Return next list node or NULL if this was the last one. */
list_node_t *list_prev(list_t *list, list_node_t *node)
{
	(void) list;
	assert(list != NULL);
	assert(node != NULL);

	return (node->prev != &list->head) ? node->prev : NULL;
}

/** Return b_true if list is empty. */
bool_t list_is_empty(list_t *list)
{
	return (list_first(list) == NULL);
}

/** Create new node. */
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

/** Delete node. */
static void list_node_delete(list_node_t *node)
{
	assert(node->prev == NULL);
	assert(node->next == NULL);
	assert(node->data == NULL);
	free(node);
}

/** Insert node between two other nodes. */
static void list_node_insert_between(list_node_t *n, list_node_t *a, list_node_t *b)
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

/** Unlink node. */
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

/** Check whether @a node is in list @a list. */
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
