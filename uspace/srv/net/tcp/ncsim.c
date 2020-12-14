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

/** @addtogroup tcp
 * @{
 */

/**
 * @file Network condition simulator
 *
 * Simulate network conditions for testing the reliability implementation:
 *    - variable latency
 *    - frame drop
 */

#include <adt/list.h>
#include <async.h>
#include <errno.h>
#include <inet/endpoint.h>
#include <io/log.h>
#include <stdlib.h>
#include <fibril.h>
#include "conn.h"
#include "ncsim.h"
#include "rqueue.h"
#include "segment.h"
#include "tcp_type.h"

static list_t sim_queue;
static fibril_mutex_t sim_queue_lock;
static fibril_condvar_t sim_queue_cv;

/** Initialize segment receive queue. */
void tcp_ncsim_init(void)
{
	list_initialize(&sim_queue);
	fibril_mutex_initialize(&sim_queue_lock);
	fibril_condvar_initialize(&sim_queue_cv);
}

/** Bounce segment through simulator into receive queue.
 *
 * @param epp	Endpoint pair, oriented for transmission
 * @param seg	Segment
 */
void tcp_ncsim_bounce_seg(inet_ep2_t *epp, tcp_segment_t *seg)
{
	tcp_squeue_entry_t *sqe;
	tcp_squeue_entry_t *old_qe;
	inet_ep2_t rident;
	link_t *link;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ncsim_bounce_seg()");
	tcp_ep2_flipped(epp, &rident);
	tcp_rqueue_insert_seg(&rident, seg);
	return;

	if (0 /* rand() % 4 == 3 */) {
		/* Drop segment */
		log_msg(LOG_DEFAULT, LVL_ERROR, "NCSim dropping segment");
		tcp_segment_delete(seg);
		return;
	}

	sqe = calloc(1, sizeof(tcp_squeue_entry_t));
	if (sqe == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating SQE.");
		return;
	}

	sqe->delay = rand() % (1000 * 1000);
	sqe->epp = *epp;
	sqe->seg = seg;

	fibril_mutex_lock(&sim_queue_lock);

	link = list_first(&sim_queue);
	while (link != NULL && sqe->delay > 0) {
		old_qe = list_get_instance(link, tcp_squeue_entry_t, link);
		if (sqe->delay < old_qe->delay)
			break;

		sqe->delay -= old_qe->delay;

		link = link->next;
		if (link == &sim_queue.head)
			link = NULL;
	}

	if (link != NULL)
		list_insert_after(&sqe->link, link);
	else
		list_append(&sqe->link, &sim_queue);

	fibril_condvar_broadcast(&sim_queue_cv);
	fibril_mutex_unlock(&sim_queue_lock);
}

/** Network condition simulator handler fibril. */
static errno_t tcp_ncsim_fibril(void *arg)
{
	link_t *link;
	tcp_squeue_entry_t *sqe;
	inet_ep2_t rident;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ncsim_fibril()");

	while (true) {
		fibril_mutex_lock(&sim_queue_lock);

		while (list_empty(&sim_queue))
			fibril_condvar_wait(&sim_queue_cv, &sim_queue_lock);

		do {
			link = list_first(&sim_queue);
			sqe = list_get_instance(link, tcp_squeue_entry_t, link);

			log_msg(LOG_DEFAULT, LVL_DEBUG, "NCSim - Sleep");
			rc = fibril_condvar_wait_timeout(&sim_queue_cv,
			    &sim_queue_lock, sqe->delay);
		} while (rc != ETIMEOUT);

		list_remove(link);
		fibril_mutex_unlock(&sim_queue_lock);

		log_msg(LOG_DEFAULT, LVL_DEBUG, "NCSim - End Sleep");
		tcp_ep2_flipped(&sqe->epp, &rident);
		tcp_rqueue_insert_seg(&rident, sqe->seg);
		free(sqe);
	}

	/* Not reached */
	return 0;
}

/** Start simulator handler fibril. */
void tcp_ncsim_fibril_start(void)
{
	fid_t fid;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ncsim_fibril_start()");

	fid = fibril_create(tcp_ncsim_fibril, NULL);
	if (fid == 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed creating ncsim fibril.");
		return;
	}

	fibril_add_ready(fid);
}

/**
 * @}
 */
