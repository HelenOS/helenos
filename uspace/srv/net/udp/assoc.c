/*
 * Copyright (c) 2015 Jiri Svoboda
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
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <inet/endpoint.h>
#include <io/log.h>
#include <nettl/amap.h>
#include <stdlib.h>

#include "assoc.h"
#include "msg.h"
#include "pdu.h"
#include "udp_type.h"

static LIST_INITIALIZE(assoc_list);
static FIBRIL_MUTEX_INITIALIZE(assoc_list_lock);
static amap_t *amap;

static udp_assoc_t *udp_assoc_find_ref(inet_ep2_t *);
static errno_t udp_assoc_queue_msg(udp_assoc_t *, inet_ep2_t *, udp_msg_t *);
static udp_assocs_dep_t *assocs_dep;

/** Initialize associations. */
errno_t udp_assocs_init(udp_assocs_dep_t *dep)
{
	errno_t rc;

	rc = amap_create(&amap);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		return ENOMEM;
	}

	assocs_dep = dep;
	return EOK;
}

/** Finalize associations. */
void udp_assocs_fini(void)
{
	assert(list_empty(&assoc_list));

	amap_destroy(amap);
	amap = NULL;
}

/** Create new association structure.
 *
 * @param epp		Endpoint pair (will be copied)
 * @param cb		Callbacks
 * @param cb_arg	Callback argument
 * @return		New association or NULL
 */
udp_assoc_t *udp_assoc_new(inet_ep2_t *epp, udp_assoc_cb_t *cb, void *cb_arg)
{
	udp_assoc_t *assoc = NULL;

	/* Allocate association structure */
	assoc = calloc(1, sizeof(udp_assoc_t));
	if (assoc == NULL)
		goto error;

	fibril_mutex_initialize(&assoc->lock);

	/* One for the user */
	refcount_init(&assoc->refcnt);

	/* Initialize receive queue */
	list_initialize(&assoc->rcv_queue);
	fibril_condvar_initialize(&assoc->rcv_queue_cv);

	if (epp != NULL)
		assoc->ident = *epp;

	assoc->cb = cb;
	assoc->cb_arg = cb_arg;
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
	refcount_up(&assoc->refcnt);
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

	if (refcount_down(&assoc->refcnt))
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
	assoc->deleted = true;
	udp_assoc_delref(assoc);
}

/** Enlist association.
 *
 * Add association to the association map.
 */
errno_t udp_assoc_add(udp_assoc_t *assoc)
{
	inet_ep2_t aepp;
	errno_t rc;

	udp_assoc_addref(assoc);
	fibril_mutex_lock(&assoc_list_lock);

	rc = amap_insert(amap, &assoc->ident, assoc, af_allow_system, &aepp);
	if (rc != EOK) {
		udp_assoc_delref(assoc);
		fibril_mutex_unlock(&assoc_list_lock);
		return rc;
	}

	assoc->ident = aepp;
	list_append(&assoc->link, &assoc_list);
	fibril_mutex_unlock(&assoc_list_lock);

	return EOK;
}

/** Delist association.
 *
 * Remove association from the association map.
 */
void udp_assoc_remove(udp_assoc_t *assoc)
{
	fibril_mutex_lock(&assoc_list_lock);
	amap_remove(amap, &assoc->ident);
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
	assoc->ident.local_link = iplink;
	fibril_mutex_unlock(&assoc->lock);
}

/** Send message to association.
 *
 * @param assoc		Association
 * @param remote	Remote endpoint or inet_addr_any/inet_port_any
 *			not to override association's remote endpoint
 * @param msg		Message
 *
 * @return		EOK on success
 *			EINVAL if remote endpoint is not set
 *			ENOMEM if out of resources
 *			EIO if no route to destination exists
 */
errno_t udp_assoc_send(udp_assoc_t *assoc, inet_ep_t *remote, udp_msg_t *msg)
{
	inet_ep2_t epp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send(%p, %p, %p)",
	    assoc, remote, msg);

	/* @a remote can be used to override the remote endpoint */
	epp = assoc->ident;
	if (!inet_addr_is_any(&remote->addr) &&
	    remote->port != inet_port_any)
		epp.remote = *remote;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send - check addr any");

	if ((inet_addr_is_any(&epp.remote.addr)) ||
	    (epp.remote.port == inet_port_any))
		return EINVAL;

	/* This association has no local address set. Need to determine one. */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send - check no local addr");
	if (inet_addr_is_any(&epp.local.addr) && !assoc->nolocal) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Determine local address.");
		rc = (*assocs_dep->get_srcaddr)(&epp.remote.addr, 0,
		    &epp.local.addr);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Cannot determine "
			    "local address.");
			return EINVAL;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send - check version");

	if (!inet_addr_is_any(&epp.local.addr) &&
	    epp.remote.addr.version != epp.local.addr.version)
		return EINVAL;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send - transmit");
	rc = (*assocs_dep->transmit_msg)(&epp, msg);

	if (rc != EOK)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send - success");
	return EOK;
}

/** Get a received message.
 *
 * Pull one message from the association's receive queue.
 */
errno_t udp_assoc_recv(udp_assoc_t *assoc, udp_msg_t **msg, inet_ep_t *remote)
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
		return ENXIO;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_recv() - got a message");
	link = list_first(&assoc->rcv_queue);
	rqe = list_get_instance(link, udp_rcv_queue_entry_t, link);
	list_remove(link);
	fibril_mutex_unlock(&assoc->lock);

	*msg = rqe->msg;
	*remote = rqe->epp.remote;
	free(rqe);

	return EOK;
}

/** Message received.
 *
 * Find the association to which the message belongs and queue it.
 */
void udp_assoc_received(inet_ep2_t *repp, udp_msg_t *msg)
{
	udp_assoc_t *assoc;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_received(%p, %p)", repp, msg);

	assoc = udp_assoc_find_ref(repp);
	if (assoc == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "No association found. Message dropped.");
		/* XXX Generate ICMP error. */
		/* XXX Might propagate error directly by error return. */
		udp_msg_delete(msg);
		return;
	}

	if (0) {
		rc = udp_assoc_queue_msg(assoc, repp, msg);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Out of memory. Message dropped.");
			/* XXX Generate ICMP error? */
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "call assoc->cb->recv_msg");
	assoc->cb->recv_msg(assoc->cb_arg, repp, msg);
	udp_assoc_delref(assoc);
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

static errno_t udp_assoc_queue_msg(udp_assoc_t *assoc, inet_ep2_t *epp,
    udp_msg_t *msg)
{
	udp_rcv_queue_entry_t *rqe;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_queue_msg(%p, %p, %p)",
	    assoc, epp, msg);

	rqe = calloc(1, sizeof(udp_rcv_queue_entry_t));
	if (rqe == NULL)
		return ENOMEM;

	link_initialize(&rqe->link);
	rqe->epp = *epp;
	rqe->msg = msg;

	fibril_mutex_lock(&assoc->lock);
	list_append(&rqe->link, &assoc->rcv_queue);
	fibril_mutex_unlock(&assoc->lock);

	fibril_condvar_broadcast(&assoc->rcv_queue_cv);

	return EOK;
}

/** Find association structure for specified endpoint pair.
 *
 * An association is uniquely identified by an endpoint pair. Look up our
 * association map and return association structure based on endpoint pair.
 * The association reference count is bumped by one.
 *
 * @param epp	Endpoint pair
 * @return	Association structure or NULL if not found.
 */
static udp_assoc_t *udp_assoc_find_ref(inet_ep2_t *epp)
{
	errno_t rc;
	void *arg;
	udp_assoc_t *assoc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_find_ref(%p)", epp);
	fibril_mutex_lock(&assoc_list_lock);

	rc = amap_find_match(amap, epp, &arg);
	if (rc != EOK) {
		assert(rc == ENOENT);
		fibril_mutex_unlock(&assoc_list_lock);
		return NULL;
	}

	assoc = (udp_assoc_t *)arg;
	udp_assoc_addref(assoc);

	fibril_mutex_unlock(&assoc_list_lock);
	return assoc;
}

/**
 * @}
 */
