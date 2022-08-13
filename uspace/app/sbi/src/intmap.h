/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INTMAP_H_
#define INTMAP_H_

#include "mytypes.h"

void intmap_init(intmap_t *intmap);
void intmap_fini(intmap_t *intmap);
void intmap_set(intmap_t *intmap, int key, void *data);
void *intmap_get(intmap_t *intmap, int key);
map_elem_t *intmap_first(intmap_t *intmap);
int intmap_elem_get_key(map_elem_t *elem);
void *intmap_elem_get_value(map_elem_t *elem);

#endif
