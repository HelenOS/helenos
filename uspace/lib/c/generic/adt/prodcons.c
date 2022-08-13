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

#include <adt/prodcons.h>
#include <adt/list.h>
#include <fibril_synch.h>

void prodcons_initialize(prodcons_t *pc)
{
	list_initialize(&pc->list);
	fibril_mutex_initialize(&pc->mtx);
	fibril_condvar_initialize(&pc->cv);
}

void prodcons_produce(prodcons_t *pc, link_t *item)
{
	fibril_mutex_lock(&pc->mtx);

	list_append(item, &pc->list);
	fibril_condvar_signal(&pc->cv);

	fibril_mutex_unlock(&pc->mtx);
}

link_t *prodcons_consume(prodcons_t *pc)
{
	fibril_mutex_lock(&pc->mtx);

	while (list_empty(&pc->list))
		fibril_condvar_wait(&pc->cv, &pc->mtx);

	link_t *head = list_first(&pc->list);
	list_remove(head);

	fibril_mutex_unlock(&pc->mtx);

	return head;
}

/** @}
 */
