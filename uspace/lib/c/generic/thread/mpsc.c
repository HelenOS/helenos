/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

#include <fibril.h>
#include <fibril_synch.h>
#include <mem.h>
#include <stdlib.h>

#include "../private/fibril.h"

/*
 * A multi-producer, single-consumer concurrent FIFO channel with unlimited
 * buffering.
 *
 * The current implementation is based on the super simple two-lock queue
 * by Michael and Scott
 * (http://www.cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf)
 *
 * The original algorithm uses one lock on each side. Since this queue is
 * single-consumer, we only use the tail lock.
 */

typedef struct mpsc_node mpsc_node_t;

struct mpsc {
	size_t elem_size;
	fibril_rmutex_t t_lock;
	mpsc_node_t *head;
	mpsc_node_t *tail;
	mpsc_node_t *close_node;
	fibril_event_t event;
};

struct mpsc_node {
	mpsc_node_t *next;
	unsigned char data[];
};

mpsc_t *mpsc_create(size_t elem_size)
{
	mpsc_t *q = calloc(1, sizeof(mpsc_t));
	mpsc_node_t *n = calloc(1, sizeof(mpsc_node_t) + elem_size);
	mpsc_node_t *c = calloc(1, sizeof(mpsc_node_t) + elem_size);

	if (!q || !n || !c) {
		free(q);
		free(n);
		free(c);
		return NULL;
	}

	if (fibril_rmutex_initialize(&q->t_lock) != EOK) {
		free(q);
		free(n);
		free(c);
		return NULL;
	}

	q->elem_size = elem_size;
	q->head = q->tail = n;
	q->close_node = c;
	return q;
}

void mpsc_destroy(mpsc_t *q)
{
	mpsc_node_t *n = q->head;
	mpsc_node_t *next = NULL;
	while (n != NULL) {
		next = n->next;
		free(n);
		n = next;
	}

	fibril_rmutex_destroy(&q->t_lock);

	free(q);
}

static errno_t _mpsc_push(mpsc_t *q, mpsc_node_t *n)
{
	fibril_rmutex_lock(&q->t_lock);

	if (q->tail == q->close_node) {
		fibril_rmutex_unlock(&q->t_lock);
		return EINVAL;
	}

	__atomic_store_n(&q->tail->next, n, __ATOMIC_RELEASE);
	q->tail = n;

	fibril_rmutex_unlock(&q->t_lock);

	fibril_notify(&q->event);
	return EOK;
}

/**
 * Send data on the channel.
 * The length of data is equal to the `elem_size` value set in `mpsc_create`.
 *
 * This function is safe for use under restricted mutex lock.
 *
 * @return ENOMEM if allocation failed, EINVAL if the queue is closed.
 */
errno_t mpsc_send(mpsc_t *q, const void *b)
{
	mpsc_node_t *n = malloc(sizeof(mpsc_node_t) + q->elem_size);
	if (!n)
		return ENOMEM;

	n->next = NULL;
	memcpy(n->data, b, q->elem_size);

	return _mpsc_push(q, n);
}

/**
 * Receive data from the channel.
 *
 * @return ETIMEOUT if deadline expires, ENOENT if the queue is closed and
 * there is no message left in the queue.
 */
errno_t mpsc_receive(mpsc_t *q, void *b, const struct timespec *expires)
{
	mpsc_node_t *n;
	mpsc_node_t *new_head;

	while (true) {
		n = q->head;
		new_head = __atomic_load_n(&n->next, __ATOMIC_ACQUIRE);
		if (new_head)
			break;

		errno_t rc = fibril_wait_timeout(&q->event, expires);
		if (rc != EOK)
			return rc;
	}

	if (new_head == q->close_node)
		return ENOENT;

	memcpy(b, new_head->data, q->elem_size);
	q->head = new_head;

	free(n);
	return EOK;
}

/**
 * Close the channel.
 *
 * This function is safe for use under restricted mutex lock.
 */
void mpsc_close(mpsc_t *q)
{
	_mpsc_push(q, q->close_node);
}
