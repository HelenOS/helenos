/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup udp
 * @{
 */

/**
 * @file UDP associations
 */

#include <adt/list.h>
#include <bool.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <stdlib.h>

#include "assoc.h"
#include "msg.h"
#include "pdu.h"
#include "ucall.h"
#include "udp_inet.h"
#include "udp_type.h"

LIST_INITIALIZE(assoc_list);
FIBRIL_MUTEX_INITIALIZE(assoc_list_lock);

/** Create new association structure.
 *
 * @param lsock		Local socket (will be deeply copied)
 * @param fsock		Foreign socket (will be deeply copied)
 * @return		New association or NULL
 */
udp_assoc_t *udp_assoc_new(udp_sock_t *lsock, udp_sock_t *fsock)
{
	udp_assoc_t *assoc = NULL;

	/* Allocate connection structure */
	assoc = calloc(1, sizeof(udp_assoc_t));
	if (assoc == NULL)
		goto error;

	fibril_mutex_initialize(&assoc->lock);

	/* One for the user */
	atomic_set(&assoc->refcnt, 1);

	/* Initialize receive queue */
	list_initialize(&assoc->rcv_queue);
	fibril_condvar_initialize(&assoc->rcv_queue_cv);

	if (lsock != NULL)
		assoc->ident.local = *lsock;
	if (fsock != NULL)
		assoc->ident.foreign = *fsock;

	return assoc;
error:
	return NULL;
}

/** Destroy association structure.
 *
 * Association structure should be destroyed when the folowing conditions
 * are met:
 * (1) user has deleted the association
 * (2) nobody is holding references to the association
 *
 * This happens when @a assoc->refcnt is zero as we count (1)
 * as an extra reference.
 *
 * @param conn		Connection
 */
static void udp_assoc_free(udp_assoc_t *assoc)
{
	log_msg(LVL_DEBUG, "%s: udp_assoc_free(%p)", assoc->name, assoc);

	while (!list_empty(&assoc->rcv_queue)) {
		link_t *link = list_first(&assoc->rcv_queue);
//		....;
		list_remove(link);
	}

	free(assoc);
}

/** Add reference to association.
 *
 * Increase association reference count by one.
 *
 * @param assoc		Association
 */
void udp_assoc_addref(udp_assoc_t *assoc)
{
	log_msg(LVL_DEBUG, "%s: upd_assoc_addref(%p)", assoc->name, assoc);
	atomic_inc(&assoc->refcnt);
}

/** Remove reference from association.
 *
 * Decrease association reference count by one.
 *
 * @param assoc		Association
 */
void udp_assoc_delref(udp_assoc_t *assoc)
{
	log_msg(LVL_DEBUG, "%s: udp_assoc_delref(%p)", assoc->name, assoc);

	if (atomic_predec(&assoc->refcnt) == 0)
		udp_assoc_free(assoc);
}

/** Delete association.
 *
 * The caller promises not make no further references to @a assoc.
 * UDP will free @a assoc eventually.
 *
 * @param assoc		Association
 */
void udp_assoc_delete(udp_assoc_t *assoc)
{
	log_msg(LVL_DEBUG, "%s: udp_assoc_delete(%p)", assoc->name, assoc);

	assert(assoc->deleted == false);
	udp_assoc_delref(assoc);
	assoc->deleted = true;
}

/** Enlist association.
 *
 * Add association to the association map.
 */
void udp_assoc_add(udp_assoc_t *assoc)
{
	udp_assoc_addref(assoc);
	fibril_mutex_lock(&assoc_list_lock);
	list_append(&assoc->link, &assoc_list);
	fibril_mutex_unlock(&assoc_list_lock);
}

/** Delist association.
 *
 * Remove association from the association map.
 */
void udp_assoc_remove(udp_assoc_t *assoc)
{
	fibril_mutex_lock(&assoc_list_lock);
	list_remove(&assoc->link);
	fibril_mutex_unlock(&assoc_list_lock);
	udp_assoc_delref(assoc);
}

/** Set foreign socket in association.
 *
 * @param assoc		Association
 * @param fsock		Foreign socket (deeply copied)
 */
void udp_assoc_set_foreign(udp_assoc_t *assoc, udp_sock_t *fsock)
{
	log_msg(LVL_DEBUG, "udp_assoc_set_foreign(%p, %p)", assoc, fsock);
	fibril_mutex_lock(&assoc->lock);
	assoc->ident.foreign = *fsock;
	fibril_mutex_unlock(&assoc->lock);
}

/** Set local socket in association.
 *
 * @param assoc		Association
 * @param fsock		Foreign socket (deeply copied)
 */
void udp_assoc_set_local(udp_assoc_t *assoc, udp_sock_t *lsock)
{
	log_msg(LVL_DEBUG, "udp_assoc_set_local(%p, %p)", assoc, lsock);
	fibril_mutex_lock(&assoc->lock);
	assoc->ident.local = *lsock;
	fibril_mutex_unlock(&assoc->lock);
}

/** Send message to association.
 *
 * @param assoc		Association
 * @param fsock		Foreign socket or NULL not to override @a assoc
 * @param msg		Message
 *
 * @return		EOK on success
 *			EINVAL if foreign socket is not set
 *			ENOMEM if out of resources
 *			EIO if no route to destination exists
 */
int udp_assoc_send(udp_assoc_t *assoc, udp_sock_t *fsock, udp_msg_t *msg)
{
	udp_pdu_t *pdu;
	udp_sockpair_t sp;
	int rc;

	/* @a fsock can be used to override the foreign socket */
	sp = assoc->ident;
	if (fsock != NULL)
		sp.foreign = *fsock;

	if (sp.foreign.addr.ipv4 == 0 || sp.foreign.port == 0)
		return EINVAL;

	rc = udp_pdu_encode(&sp, msg, &pdu);
	if (rc != EOK)
		return ENOMEM;

	rc = udp_transmit_pdu(pdu);
	if (rc != EOK)
		return EIO;

	udp_pdu_delete(pdu);

	return EOK;
}

/**
 * @}
 */
