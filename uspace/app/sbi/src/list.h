/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIST_H_
#define LIST_H_

#include "mytypes.h"
#include "compat.h"

void list_init(list_t *list);
void list_fini(list_t *list);
void list_append(list_t *list, void *data);
void list_prepend(list_t *list, void *data);
void list_remove(list_t *list, list_node_t *node);

list_node_t *list_first(list_t *list);
list_node_t *list_last(list_t *list);
list_node_t *list_next(list_t *list, list_node_t *node);
list_node_t *list_prev(list_t *list, list_node_t *node);
bool_t list_is_empty(list_t *list);

void list_node_setdata(list_node_t *node, void *data);

#define list_node_data(node, dtype) ((dtype)(node->data))

#endif
