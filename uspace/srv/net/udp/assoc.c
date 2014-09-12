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
#include <stdbool.h>
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

static udp_assoc_t *udp_assoc_find_ref(udp_sockpair_t *);
static int udp_assoc_queue_msg(udp_assoc_t *, udp_sockpair_t *, udp_msg_t *);
static bool udp_socket_match(udp_sock_t *, udp_sock_t *);
static bool udp_sockpair_match(udp_sockpair_t *, udp_sockpair_t *);

/** Create new association structure.
 *
 * @param lsock		Local socket (will be deeply copied)
 * @param fsock		Foreign socket (will be deeply copied)
 * @return		New association or NULL
 */
udp_assoc_t *udp_assoc_new(udp_sock_t *lsock, udp_sock_t *fsock)
{
	udp_assoc_t *assoc = NULL;

	/* Allocate association structure */
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
 * @param assoc		Association
 */
static void udp_assoc_free(udp_assoc_t *assoc)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: udp_assoc_free(%p)", assoc->name, assoc);

	while (!list_empty(&assoc->rcv_queue)) {
		link_t *link = list_first(&assoc->rcv_queue);
		udp_rcv_queue_entry_t *rqe = list_get_instance(link,
		    udp_rcv_queue_entry_t, link);
		list_remove(link);

		udp_msg_delete(rqe->msg);
		free(rqe);
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: upd_assoc_addref(%p)", assoc->name, assoc);
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: udp_assoc_delref(%p)", assoc->name, assoc);

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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: udp_assoc_delete(%p)", assoc->name, assoc);

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

/** Set IP link in association.
 *
 * @param assoc		Association
 * @param iplink	IP link
 */
void udp_assoc_set_iplink(udp_assoc_t *assoc, service_id_t iplink)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_set_iplink(%p, %zu)",
	    assoc, iplink);
	fibril_mutex_lock(&assoc->lock);
	assoc->ident.iplink = iplink;
	fibril_mutex_unlock(&assoc->lock);
}

/** Set foreign socket in association.
 *
 * @param assoc		Association
 * @param fsock		Foreign socket (deeply copied)
 */
void udp_assoc_set_foreign(udp_assoc_t *assoc, udp_sock_t *fsock)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_set_foreign(%p, %p)", assoc, fsock);
	fibril_mutex_lock(&assoc->lock);
	assoc->ident.foreign = *fsock;
	fibril_mutex_unlock(&assoc->lock);
}

/** Set local socket in association.
 *
 * @param assoc Association
 * @param lsock Local socket (deeply copied)
 *
 */
void udp_assoc_set_local(udp_assoc_t *assoc, udp_sock_t *lsock)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_set_local(%p, %p)", assoc, lsock);
	fibril_mutex_lock(&assoc->lock);
	assoc->ident.local = *lsock;
	fibril_mutex_unlock(&assoc->lock);
}

/** Set local port in association.
 *
 * @param assoc Association
 * @param lport Local port
 *
 */
void udp_assoc_set_local_port(udp_assoc_t *assoc, uint16_t lport)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_set_local(%p, %" PRIu16 ")", assoc, lport);
	fibril_mutex_lock(&assoc->lock);
	assoc->ident.local.port = lport;
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

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send(%p, %p, %p)",
	    assoc, fsock, msg);

	/* @a fsock can be used to override the foreign socket */
	sp = assoc->ident;
	if (fsock != NULL)
		sp.foreign = *fsock;

	if ((inet_addr_is_any(&sp.foreign.addr)) ||
	    (sp.foreign.port == UDP_PORT_ANY))
		return EINVAL;

	rc = udp_pdu_encode(&sp, msg, &pdu);
	if (rc != EOK)
		return ENOMEM;

	rc = udp_transmit_pdu(pdu);
	udp_pdu_delete(pdu);

	if (rc != EOK)
		return EIO;

	return EOK;
}

/** Get a received message.
 *
 * Pull one message from the association's receive queue.
 */
int udp_assoc_recv(udp_assoc_t *assoc, udp_msg_t **msg, udp_sock_t *fsock)
{
	link_t *link;
	udp_rcv_queue_entry_t *rqe;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_recv()");

	fibril_mutex_lock(&assoc->lock);
	while (list_empty(&assoc->rcv_queue) && !assoc->reset) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_recv() - waiting");
		fibril_condvar_wait(&assoc->rcv_queue_cv, &assoc->lock);
	}

	if (assoc->reset) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_recv() - association was reset");
		fibril_mutex_unlock(&assoc->lock);
		return ECONNABORTED;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_recv() - got a message");
	link = list_first(&assoc->rcv_queue);
	rqe = list_get_instance(link, udp_rcv_queue_entry_t, link);
	list_remove(link);
	fibril_mutex_unlock(&assoc->lock);

	*msg = rqe->msg;
	*fsock = rqe->sp.foreign;
	free(rqe);

	return EOK;
}

/** Message received.
 *
 * Find the association to which the message belongs and queue it.
 */
void udp_assoc_received(udp_sockpair_t *rsp, udp_msg_t *msg)
{
	udp_assoc_t *assoc;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_received(%p, %p)", rsp, msg);

	assoc = udp_assoc_find_ref(rsp);
	if (assoc == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "No association found. Message dropped.");
		/* XXX Generate ICMP error. */
		/* XXX Might propagate error directly by error return. */
		udp_msg_delete(msg);
		return;
	}

	rc = udp_assoc_queue_msg(assoc, rsp, msg);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Out of memory. Message dropped.");
		/* XXX Generate ICMP error? */
	}
}

/** Reset association.
 *
 * This causes any pendingreceive operations to return immediately with
 * UDP_ERESET.
 */
void udp_assoc_reset(udp_assoc_t *assoc)
{
	fibril_mutex_lock(&assoc->lock);
	assoc->reset = true;
	fibril_condvar_broadcast(&assoc->rcv_queue_cv);
	fibril_mutex_unlock(&assoc->lock);
}

static int udp_assoc_queue_msg(udp_assoc_t *assoc, udp_sockpair_t *sp,
    udp_msg_t *msg)
{
	udp_rcv_queue_entry_t *rqe;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_queue_msg(%p, %p, %p)",
	    assoc, sp, msg);

	rqe = calloc(1, sizeof(udp_rcv_queue_entry_t));
	if (rqe == NULL)
		return ENOMEM;

	link_initialize(&rqe->link);
	rqe->sp = *sp;
	rqe->msg = msg;

	fibril_mutex_lock(&assoc->lock);
	list_append(&rqe->link, &assoc->rcv_queue);
	fibril_mutex_unlock(&assoc->lock);

	fibril_condvar_broadcast(&assoc->rcv_queue_cv);

	return EOK;
}

/** Match socket with pattern. */
static bool udp_socket_match(udp_sock_t *sock, udp_sock_t *patt)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "udp_socket_match(sock=(%u), pat=(%u))", sock->port, patt->port);
	
	if ((!inet_addr_is_any(&patt->addr)) &&
	    (!inet_addr_compare(&patt->addr, &sock->addr)))
		return false;
	
	if ((patt->port != UDP_PORT_ANY) &&
	    (patt->port != sock->port))
		return false;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, " -> match");
	
	return true;
}

/** Match socket pair with pattern. */
static bool udp_sockpair_match(udp_sockpair_t *sp, udp_sockpair_t *pattern)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sockpair_match(%p, %p)", sp, pattern);

	if (!udp_socket_match(&sp->local, &pattern->local))
		return false;

	if (!udp_socket_match(&sp->foreign, &pattern->foreign))
		return false;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Socket pair matched.");
	return true;
}


/** Find association structure for specified socket pair.
 *
 * An association is uniquely identified by a socket pair. Look up our
 * association map and return association structure based on socket pair.
 * The association reference count is bumped by one.
 *
 * @param sp	Socket pair
 * @return	Association structure or NULL if not found.
 */
static udp_assoc_t *udp_assoc_find_ref(udp_sockpair_t *sp)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_find_ref(%p)", sp);
	
	fibril_mutex_lock(&assoc_list_lock);
	
	list_foreach(assoc_list, link, udp_assoc_t, assoc) {
		udp_sockpair_t *asp = &assoc->ident;
		
		/* Skip unbound associations */
		if (asp->local.port == UDP_PORT_ANY)
			continue;
		
		if (udp_sockpair_match(sp, asp)) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Returning assoc %p", assoc);
			udp_assoc_addref(assoc);
			fibril_mutex_unlock(&assoc_list_lock);
			return assoc;
		}
	}
	
	fibril_mutex_unlock(&assoc_list_lock);
	return NULL;
}

/**
 * @}
 */
