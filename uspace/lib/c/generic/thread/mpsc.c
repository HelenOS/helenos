/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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
