/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIST_T_H_
#define LIST_T_H_

#include "compat.h"

typedef struct list_node {
	struct list_node *prev, *next;
	void *data;
} list_node_t;

typedef struct list {
	/** Empty head (no data) */
	list_node_t head;
} list_t;

#endif
