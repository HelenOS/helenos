/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INTMAP_T_H_
#define INTMAP_T_H_

#include "list_t.h"

typedef struct {
	int key;
	void *value;
} map_elem_t;

typedef struct intmap {
	list_t elem; /* of (map_elem_t *) */
} intmap_t;

#endif
