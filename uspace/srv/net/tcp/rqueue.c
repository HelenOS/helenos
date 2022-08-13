/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */

/**
 * @file Global segment receive queue
 */

#include <adt/prodcons.h>
#include <errno.h>
#include <io/log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fibril.h>
#include <fibril_synch.h>
#include "conn.h"
#include "rqueue.h"
#include "segment.h"
#include "tcp_type.h"
#include "ucall.h"

static prodcons_t rqueue;
static bool fibril_active;
static fibril_mutex_t lock;
static fibril_condvar_t cv;
static tcp_rqueue_cb_t *rqueue_cb;

/** Initialize segment receive queue. */
void tcp_rqueue_init(tcp_rqueue_cb_t *rcb)
{
	prodcons_initialize(&rqueue);
	fibril_mutex_initialize(&lock);
	fibril_condvar_initialize(&cv);
	fibril_active = false;
	rqueue_cb = rcb;
}

/** Finalize segment receive queue. */
void tcp_rqueue_fini(void)
{
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	tcp_rqueue_insert_seg(&epp, NULL);

	fibril_mutex_lock(&lock);
	while (fibril_active)
		fibril_condvar_wait(&cv, &lock);
	fibril_mutex_unlock(&lock);
}

/** Insert segment into receive queue.
 *
 * @param epp	Endpoint pair, oriented for reception
 * @param seg	Segment (ownership transferred to rqueue)
 */
void tcp_rqueue_insert_seg(inet_ep2_t *epp, tcp_segment_t *seg)
{
	tcp_rqueue_entry_t *rqe;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "tcp_rqueue_insert_seg()");

	if (seg != NULL)
		tcp_segment_dump(seg);

	rqe = calloc(1, sizeof(tcp_rqueue_entry_t));
	if (rqe == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating RQE.");
		return;
	}

	rqe->epp = *epp;
	rqe->seg = seg;

	prodcons_produce(&rqueue, &rqe->link);
}

/** Receive queue handler fibril. */
static errno_t tcp_rqueue_fibril(void *arg)
{
	link_t *link;
	tcp_rqueue_entry_t *rqe;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_rqueue_fibril()");

	while (true) {
		link = prodcons_consume(&rqueue);
		rqe = list_get_instance(link, tcp_rqueue_entry_t, link);

		if (rqe->seg == NULL) {
			free(rqe);
			break;
		}

		rqueue_cb->seg_received(&rqe->epp, rqe->seg);
		free(rqe);
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "tcp_rqueue_fibril() exiting");

	/* Finished */
	fibril_mutex_lock(&lock);
	fibril_active = false;
	fibril_mutex_unlock(&lock);
	fibril_condvar_broadcast(&cv);

	return 0;
}

/** Start receive queue handler fibril. */
void tcp_rqueue_fibril_start(void)
{
	fid_t fid;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_rqueue_fibril_start()");

	fid = fibril_create(tcp_rqueue_fibril, NULL);
	if (fid == 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed creating rqueue fibril.");
		return;
	}

	fibril_add_ready(fid);
	fibril_active = true;
}

/**
 * @}
 */
