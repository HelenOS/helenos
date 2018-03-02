/*
 * Copyright (c) 2011 Martin Decky
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
