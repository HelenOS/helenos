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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief Datagram reassembly.
 */

#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>

#include "inetsrv.h"
#include "inet_std.h"
#include "reass.h"

/** Datagram being reassembled.
 *
 * Uniquely identified by (source address, destination address, protocol,
 * identification) per RFC 791 sec. 2.3 / Fragmentation.
 */
typedef struct {
	link_t map_link;
	/** List of fragments, @c reass_frag_t */
	list_t frags;
} reass_dgram_t;

/** One datagram fragment */
typedef struct {
	link_t dgram_link;
	inet_packet_t packet;
} reass_frag_t;

/** Datagram map, list of reass_dgram_t */
static LIST_INITIALIZE(reass_dgram_map);
/** Protects access to @c reass_dgram_map */
static FIBRIL_MUTEX_INITIALIZE(reass_dgram_map_lock);

static reass_dgram_t *reass_dgram_new(void);
static reass_dgram_t *reass_dgram_get(inet_packet_t *);
static errno_t reass_dgram_insert_frag(reass_dgram_t *, inet_packet_t *);
static bool reass_dgram_complete(reass_dgram_t *);
static void reass_dgram_remove(reass_dgram_t *);
static errno_t reass_dgram_deliver(reass_dgram_t *);
static void reass_dgram_destroy(reass_dgram_t *);

/** Queue packet for datagram reassembly.
 *
 * @param packet	Packet
 * @return		EOK on success or ENOMEM.
 */
errno_t inet_reass_queue_packet(inet_packet_t *packet)
{
	reass_dgram_t *rdg;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_reass_queue_packet()");

	fibril_mutex_lock(&reass_dgram_map_lock);

	/* Get existing or new datagram */
	rdg = reass_dgram_get(packet);
	if (rdg == NULL) {
		/* Only happens when we are out of memory */
		fibril_mutex_unlock(&reass_dgram_map_lock);
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Allocation failed, packet dropped.");
		return ENOMEM;
	}

	/* Insert fragment into the datagram */
	rc = reass_dgram_insert_frag(rdg, packet);
	if (rc != EOK)
		return ENOMEM;

	/* Check if datagram is complete */
	if (reass_dgram_complete(rdg)) {
		/* Remove it from the map */
		reass_dgram_remove(rdg);
		fibril_mutex_unlock(&reass_dgram_map_lock);

		/* Deliver complete datagram */
		rc = reass_dgram_deliver(rdg);
		reass_dgram_destroy(rdg);
		return rc;
	}

	fibril_mutex_unlock(&reass_dgram_map_lock);
	return EOK;
}

/** Get datagram reassembly structure for packet.
 *
 * @param packet	Packet
 * @return		Datagram reassembly structure matching @a packet
 */
static reass_dgram_t *reass_dgram_get(inet_packet_t *packet)
{
	assert(fibril_mutex_is_locked(&reass_dgram_map_lock));

	list_foreach(reass_dgram_map, map_link, reass_dgram_t, rdg) {
		link_t *f1_link = list_first(&rdg->frags);
		assert(f1_link != NULL);

		reass_frag_t *f1 = list_get_instance(f1_link, reass_frag_t,
		    dgram_link);

		if ((inet_addr_compare(&f1->packet.src, &packet->src)) &&
		    (inet_addr_compare(&f1->packet.dest, &packet->dest)) &&
		    (f1->packet.proto == packet->proto) &&
		    (f1->packet.ident == packet->ident)) {
			/* Match */
			return rdg;
		}
	}

	/* No existing reassembly structure. Create a new one. */
	return reass_dgram_new();
}

/** Create new datagram reassembly structure.
 *
 * @return New datagram reassembly structure.
 */
static reass_dgram_t *reass_dgram_new(void)
{
	reass_dgram_t *rdg;

	rdg = calloc(1, sizeof(reass_dgram_t));
	if (rdg == NULL)
		return NULL;

	list_append(&rdg->map_link, &reass_dgram_map);
	list_initialize(&rdg->frags);

	return rdg;
}

static reass_frag_t *reass_frag_new(void)
{
	reass_frag_t *frag;

	frag = calloc(1, sizeof(reass_frag_t));
	if (frag == NULL)
		return NULL;

	link_initialize(&frag->dgram_link);

	return frag;
}

static errno_t reass_dgram_insert_frag(reass_dgram_t *rdg, inet_packet_t *packet)
{
	reass_frag_t *frag;
	void *data_copy;
	link_t *link;

	assert(fibril_mutex_is_locked(&reass_dgram_map_lock));

	frag = reass_frag_new();

	/* Clone the packet */

	data_copy = malloc(packet->size);
	if (data_copy == NULL) {
		free(frag);
		return ENOMEM;
	}

	memcpy(data_copy, packet->data, packet->size);

	frag->packet = *packet;
	frag->packet.data = data_copy;

	/*
	 * XXX Make resource-consuming attacks harder, eliminate any duplicate
	 * data immediately. Possibly eliminate redundant packet headers.
	 */

	/*
	 * Insert into the list, which is sorted by offs member ascending.
	 */

	link = list_first(&rdg->frags);
	while (link != NULL) {
		reass_frag_t *qf = list_get_instance(link, reass_frag_t,
		    dgram_link);

		if (qf->packet.offs >= packet->offs)
			break;

		link = list_next(link, &rdg->frags);
	}

	if (link != NULL)
		list_insert_after(&frag->dgram_link, link);
	else
		list_append(&frag->dgram_link, &rdg->frags);

	return EOK;
}

/** Check if datagram is complete.
 *
 * @param rdg		Datagram reassembly structure
 * @return		@c true if complete, @c false if not
 */
static bool reass_dgram_complete(reass_dgram_t *rdg)
{
	reass_frag_t *frag, *prev;
	link_t *link;

	assert(fibril_mutex_is_locked(&reass_dgram_map_lock));
	assert(!list_empty(&rdg->frags));

	link = list_first(&rdg->frags);
	assert(link != NULL);

	frag = list_get_instance(link, reass_frag_t,
	    dgram_link);

	/* First fragment must be at offset zero */
	if (frag->packet.offs != 0)
		return false;

	prev = frag;

	while (true) {
		link = list_next(link, &rdg->frags);
		if (link == NULL)
			break;

		/* Each next fragment must follow immediately or overlap */
		frag = list_get_instance(link, reass_frag_t, dgram_link);
		if (frag->packet.offs > prev->packet.offs + prev->packet.size)
			return false;

		/* No more fragments - datagram is complete */
		if (!frag->packet.mf)
			return true;

		prev = frag;
	}

	return false;
}

/** Remove datagram from reassembly map.
 *
 * @param rdg		Datagram reassembly structure
 */
static void reass_dgram_remove(reass_dgram_t *rdg)
{
	assert(fibril_mutex_is_locked(&reass_dgram_map_lock));
	list_remove(&rdg->map_link);
}

/** Deliver complete datagram.
 *
 * @param rdg		Datagram reassembly structure.
 */
static errno_t reass_dgram_deliver(reass_dgram_t *rdg)
{
	size_t dgram_size;
	size_t fragoff_limit;
	inet_dgram_t dgram;
	uint8_t proto;
	reass_frag_t *frag;
	errno_t rc;

	/*
	 * Potentially there could be something beyond the first packet
	 * that has !MF. Make sure we ignore that.
	 */
	frag = NULL;
	list_foreach(rdg->frags, dgram_link, reass_frag_t, cfrag) {
		if (!cfrag->packet.mf) {
			frag = cfrag;
			break;
		}
	}

	assert(frag != NULL);
	assert(!frag->packet.mf);

	dgram_size = frag->packet.offs + frag->packet.size;

	/* Upper bound for fragment offset field */
	fragoff_limit = 1 << (FF_FRAGOFF_h - FF_FRAGOFF_l + 1);

	/* Verify that total size of datagram is within reasonable bounds */
	if (dgram_size > FRAG_OFFS_UNIT * fragoff_limit)
		return ELIMIT;

	dgram.data = calloc(dgram_size, 1);
	if (dgram.data == NULL)
		return ENOMEM;

	/* XXX What if different fragments came from different link? */
	dgram.iplink = frag->packet.link_id;
	dgram.size = dgram_size;
	dgram.src = frag->packet.src;
	dgram.dest = frag->packet.dest;
	dgram.tos = frag->packet.tos;
	proto = frag->packet.proto;

	/* Pull together data from individual fragments */

	size_t doffs = 0;

	list_foreach(rdg->frags, dgram_link, reass_frag_t, cfrag) {
		size_t cb, ce;

		cb = max(doffs, cfrag->packet.offs);
		ce = min(dgram_size, cfrag->packet.offs + cfrag->packet.size);

		if (ce > cb) {
			memcpy(dgram.data + cb,
			    cfrag->packet.data + cb - cfrag->packet.offs,
			    ce - cb);
		}

		if (!cfrag->packet.mf)
			break;
	}

	rc = inet_recv_dgram_local(&dgram, proto);
	free(dgram.data);
	return rc;
}

/** Destroy datagram reassembly structure.
 *
 * @param rdg		Datagram reassembly structure.
 */
static void reass_dgram_destroy(reass_dgram_t *rdg)
{
	while (!list_empty(&rdg->frags)) {
		link_t *flink = list_first(&rdg->frags);
		reass_frag_t *frag = list_get_instance(flink, reass_frag_t,
		    dgram_link);

		list_remove(&frag->dgram_link);
		free(frag->packet.data);
		free(frag);
	}

	free(rdg);
}

/** @}
 */
