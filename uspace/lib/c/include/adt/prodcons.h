/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRODCONS_H_
#define _LIBC_PRODCONS_H_

#include <adt/list.h>
#include <fibril_synch.h>

typedef struct {
	fibril_mutex_t mtx;
	fibril_condvar_t cv;
	list_t list;
} prodcons_t;

extern void prodcons_initialize(prodcons_t *);
extern void prodcons_produce(prodcons_t *, link_t *);
extern link_t *prodcons_consume(prodcons_t *);

#endif

/** @}
 */
