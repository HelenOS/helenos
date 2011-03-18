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

/** @file Integer map.
 *
 * Maps integers to pointers (void *). Current implementation is trivial
 * (linked list of key-value pairs).
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#include "mytypes.h"

#include "intmap.h"

/** Initialize map.
 *
 * @param intmap	Map to initialize.
 */
void intmap_init(intmap_t *intmap)
{
	list_init(&intmap->elem);
}

/** Deinitialize map.
 *
 * The map must be already empty.
 *
 * @param intmap	Map to initialize.
 */
void intmap_fini(intmap_t *intmap)
{
	list_fini(&intmap->elem);
}

/** Set value corresponding to a key.
 *
 * If there already exists a mapping for @a key in the map, it is
 * silently replaced. If @a value is @c NULL, the mapping for @a key
 * is removed from the map.
 *
 * @param intmap	Map
 * @param key		Key (integer)
 * @param value		Value (must be a pointer) or @c NULL
 */
void intmap_set(intmap_t *intmap, int key, void *value)
{
	list_node_t *node;
	map_elem_t *elem;

	node = list_first(&intmap->elem);
	while (node != NULL) {
		elem = list_node_data(node, map_elem_t *);
		if (elem->key == key) {
			if (value != NULL) {
				/* Replace existing value. */
				elem->value = value;
			} else {
				/* Remove map element. */
				list_remove(&intmap->elem, node);
				free(elem);
			}
			return;
		}
		node = list_next(&intmap->elem, node);
	}

	/* Allocate new map element and add it to the list. */

	elem = calloc(1, sizeof(map_elem_t));
	if (elem == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	elem->key = key;
	elem->value = value;
	list_append(&intmap->elem, elem);
}

/** Get value corresponding to a key.
 *
 * @param intmap	Map
 * @param key		Key for which to retrieve mapping
 *
 * @return		Value correspoding to @a key or @c NULL if no mapping
 *			exists.
 */
void *intmap_get(intmap_t *intmap, int key)
{
	list_node_t *node;
	map_elem_t *elem;

	node = list_first(&intmap->elem);
	while (node != NULL) {
		elem = list_node_data(node, map_elem_t *);
		if (elem->key == key) {
			return elem->value;
		}
		node = list_next(&intmap->elem, node);
	}

	/* Not found */
	return NULL;
}

/** Get first element in the map.
 *
 * For iterating over the map, this returns the first element (in no
 * particular order).
 *
 * @param intmap	Map
 * @return		Map element or NULL if the map is empty
 */
map_elem_t *intmap_first(intmap_t *intmap)
{
	list_node_t *node;

	node = list_first(&intmap->elem);
	if (node == NULL)
		return NULL;

	return list_node_data(node, map_elem_t *);
}

/** Get element key.
 *
 * Giver a map element, return the key.
 *
 * @param elem		Map element
 * @return		Key
 */
int intmap_elem_get_key(map_elem_t *elem)
{
	return elem->key;
}

/** Get element value.
 *
 * Giver a map element, return the value.
 *
 * @param elem		Map element
 * @return		Value
 */
void *intmap_elem_get_value(map_elem_t *elem)
{
	return elem->value;
}
