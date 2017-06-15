/*
 * Copyright (c) 2017 Ondrej Hlavaty
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

#include <errno.h>
#include <assert.h>
#include <ddi.h>
#include <as.h>
#include <align.h>
#include "trb_ring.h"

#define SEGMENT_HEADER_SIZE (sizeof(link_t) + sizeof(uintptr_t))

/**
 * Number of TRBs in a segment (with our header).
 */
#define SEGMENT_TRB_COUNT ((PAGE_SIZE - SEGMENT_HEADER_SIZE) / sizeof(xhci_trb_t))

struct trb_segment {
	xhci_trb_t trb_storage [SEGMENT_TRB_COUNT] __attribute__((packed));

	link_t segments_link;
	uintptr_t phys;
} __attribute__((aligned(PAGE_SIZE)));


static inline xhci_trb_t *segment_begin(trb_segment_t *segment)
{
	return segment->trb_storage;
}

static inline xhci_trb_t *segment_end(trb_segment_t *segment)
{
	return segment_begin(segment) + SEGMENT_TRB_COUNT;
}

static int trb_segment_allocate(trb_segment_t **segment)
{
	uintptr_t phys;
	int err;

	*segment = AS_AREA_ANY;
	err = dmamem_map_anonymous(PAGE_SIZE,
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0, &phys,
	    (void *) segment);

	if (err == EOK) {
		memset(*segment, 0, PAGE_SIZE);
		(*segment)->phys = phys;
	}

	return err;
}

int trb_ring_init(xhci_trb_ring_t *ring)
{
	struct trb_segment *segment;
	int err;

	if ((err = trb_segment_allocate(&segment)) != EOK)
		return err;

	list_initialize(&ring->segments);
	list_append(&segment->segments_link, &ring->segments);

	xhci_trb_t *last = segment_end(segment) - 1;
	xhci_trb_link_fill(last, segment->phys);
	xhci_trb_set_cycle(last, true);

	ring->enqueue_segment = segment;
	ring->enqueue_trb = segment_begin(segment);
	ring->dequeue = segment->phys;
	ring->pcs = 1;

	return EOK;
}

int trb_ring_fini(xhci_trb_ring_t *ring)
{
	return EOK;
}

/**
 * When the enqueue pointer targets a Link TRB, resolve it.
 *
 * Relies on segments being in the segment list in linked order.
 *
 * According to section 4.9.2.2, figure 16, the link TRBs cannot be chained, so
 * it shall not be called in cycle, nor have an inner cycle.
 */
static void trb_ring_resolve_link(xhci_trb_ring_t *ring)
{
	link_t *next_segment = list_next(&ring->enqueue_segment->segments_link, &ring->segments);
	if (!next_segment)
		next_segment = list_first(&ring->segments);

	ring->enqueue_segment = list_get_instance(next_segment, trb_segment_t, segments_link);
	ring->enqueue_trb = segment_begin(ring->enqueue_segment);
}

static uintptr_t xhci_trb_ring_enqueue_phys(xhci_trb_ring_t *ring)
{
	uintptr_t trb_id = ring->enqueue_trb - segment_begin(ring->enqueue_segment);
	return ring->enqueue_segment->phys + trb_id * sizeof(xhci_trb_t);
}

int trb_ring_enqueue(xhci_trb_ring_t *ring, xhci_trb_t *td)
{
	xhci_trb_t * const saved_enqueue_trb = ring->enqueue_trb;
	trb_segment_t * const saved_enqueue_segment = ring->enqueue_segment;

	/*
	 * First, dry run and advance the enqueue pointer to see if the ring would
	 * be full anytime during the transaction.
	 */
	xhci_trb_t *trb = td;
	do {
		ring->enqueue_trb++;

		if (TRB_TYPE(*ring->enqueue_trb) == XHCI_TRB_TYPE_LINK)
			trb_ring_resolve_link(ring);

		if (xhci_trb_ring_enqueue_phys(ring) == ring->dequeue)
			goto err_again;
	} while (xhci_trb_is_chained(trb++));

	ring->enqueue_segment = saved_enqueue_segment;
	ring->enqueue_trb = saved_enqueue_trb;

	/*
	 * Now, copy the TRBs without further checking.
	 */
	trb = td;
	do {
		xhci_trb_set_cycle(trb, ring->pcs);
		xhci_trb_copy(ring->enqueue_trb, trb);

		ring->enqueue_trb++;

		if (TRB_TYPE(*ring->enqueue_trb) == XHCI_TRB_TYPE_LINK) {
			// XXX: Check, whether the order here is correct (ambiguous instructions in 4.11.5.1)
			xhci_trb_set_cycle(ring->enqueue_trb, ring->pcs);

			if (TRB_LINK_TC(*ring->enqueue_trb))
				ring->pcs = !ring->pcs;

			trb_ring_resolve_link(ring);
		}
	} while (xhci_trb_is_chained(trb++));

	return EOK;

err_again:
	ring->enqueue_segment = saved_enqueue_segment;
	ring->enqueue_trb = saved_enqueue_trb;
	return EAGAIN;
}
