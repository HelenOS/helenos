/*
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek, Jaroslav Jindrak, Jan Hrach
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
#include <libarch/barrier.h>
#include <usb/debug.h>
#include "hw_struct/trb.h"
#include "trb_ring.h"

/**
 * A structure representing a segment of a TRB ring.
 */

#define SEGMENT_FOOTER_SIZE (sizeof(link_t) + sizeof(uintptr_t))

#define SEGMENT_TRB_COUNT ((PAGE_SIZE - SEGMENT_FOOTER_SIZE) / sizeof(xhci_trb_t))
#define SEGMENT_TRB_USEFUL_COUNT (SEGMENT_TRB_COUNT - 1)

struct trb_segment {
	xhci_trb_t trb_storage [SEGMENT_TRB_COUNT];

	link_t segments_link;
	uintptr_t phys;
} __attribute__((aligned(PAGE_SIZE)));

static_assert(sizeof(trb_segment_t) == PAGE_SIZE);


/**
 * Get the first TRB of a segment.
 */
static inline xhci_trb_t *segment_begin(trb_segment_t *segment)
{
	return segment->trb_storage;
}

/**
 * Get the one-past-end TRB of a segment.
 */
static inline xhci_trb_t *segment_end(trb_segment_t *segment)
{
	return segment_begin(segment) + SEGMENT_TRB_COUNT;
}

/**
 * Return a first segment of a list of segments.
 */
static inline trb_segment_t *get_first_segment(list_t *segments)
{
	return list_get_instance(list_first(segments), trb_segment_t, segments_link);

}

/**
 * Allocate and initialize new segment.
 *
 * TODO: When the HC supports 64-bit addressing, there's no need to restrict
 * to DMAMEM_4GiB.
 */
static errno_t trb_segment_alloc(trb_segment_t **segment)
{
	*segment = AS_AREA_ANY;
	uintptr_t phys;

	const int err = dmamem_map_anonymous(PAGE_SIZE,
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &phys, (void **) segment);
	if (err)
		return err;

	memset(*segment, 0, PAGE_SIZE);
	(*segment)->phys = phys;
	usb_log_debug("Allocated new ring segment.");
	return EOK;
}

static void trb_segment_free(trb_segment_t *segment)
{
	dmamem_unmap_anonymous(segment);
}

/**
 * Initializes the ring with one segment.
 *
 * @param[in] initial_size A number of free slots on the ring, 0 leaves the
 * choice on a reasonable default (one page-sized segment).
 */
errno_t xhci_trb_ring_init(xhci_trb_ring_t *ring, size_t initial_size)
{
	errno_t err;
	if (initial_size == 0)
		initial_size = SEGMENT_TRB_USEFUL_COUNT;

	list_initialize(&ring->segments);
	size_t segment_count = (initial_size + SEGMENT_TRB_USEFUL_COUNT - 1)
		/ SEGMENT_TRB_USEFUL_COUNT;

	for (size_t i = 0; i < segment_count; ++i) {
		struct trb_segment *segment;
		if ((err = trb_segment_alloc(&segment)) != EOK)
			return err;

		list_append(&segment->segments_link, &ring->segments);
		ring->segment_count = i + 1;
	}

	trb_segment_t * const segment = get_first_segment(&ring->segments);
	xhci_trb_t *last = segment_end(segment) - 1;
	xhci_trb_link_fill(last, segment->phys);
	TRB_LINK_SET_TC(*last, true);

	ring->enqueue_segment = segment;
	ring->enqueue_trb = segment_begin(segment);
	ring->dequeue = segment->phys;
	ring->pcs = 1;

	fibril_mutex_initialize(&ring->guard);

	return EOK;
}

/**
 * Free all segments inside the ring.
 */
void xhci_trb_ring_fini(xhci_trb_ring_t *ring)
{
	assert(ring);

	list_foreach_safe(ring->segments, cur, next) {
		trb_segment_t *segment =
		    list_get_instance(cur, trb_segment_t, segments_link);
		trb_segment_free(segment);
	}
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
	link_t *next_segment =
	    list_next(&ring->enqueue_segment->segments_link, &ring->segments);
	if (!next_segment)
		next_segment = list_first(&ring->segments);
	assert(next_segment);

	ring->enqueue_segment =
	    list_get_instance(next_segment, trb_segment_t, segments_link);
	ring->enqueue_trb = segment_begin(ring->enqueue_segment);
}

/**
 * Get the physical address of the enqueue pointer.
 */
static uintptr_t trb_ring_enqueue_phys(xhci_trb_ring_t *ring)
{
	size_t trb_id = ring->enqueue_trb - segment_begin(ring->enqueue_segment);
	return ring->enqueue_segment->phys + trb_id * sizeof(xhci_trb_t);
}

/**
 * Decides whether the TRB will trigger an interrupt after being processed.
 */
static bool trb_generates_interrupt(xhci_trb_t *trb)
{
	return TRB_TYPE(*trb) >= XHCI_TRB_TYPE_ENABLE_SLOT_CMD
		|| TRB_IOC(*trb);
}

/**
 * Enqueue TD composed of TRBs.
 *
 * This will copy specified number of TRBs chained together into the ring. The
 * cycle flag in TRBs may be changed.
 *
 * The copied TRBs must be contiguous in memory, and must not contain Link TRBs.
 *
 * We cannot avoid the copying, because the TRB in ring should be updated
 * atomically.
 *
 * @param first_trb the first TRB
 * @param trbs number of TRBS to enqueue
 * @param phys returns address of the last TRB enqueued
 * @return EOK on success,
 *         EAGAIN when the ring is too full to fit all TRBs (temporary)
 */
errno_t xhci_trb_ring_enqueue_multiple(xhci_trb_ring_t *ring, xhci_trb_t *first_trb,
	size_t trbs, uintptr_t *phys)
{
	errno_t err;
	assert(trbs > 0);

	if (trbs > xhci_trb_ring_size(ring))
		return ELIMIT;

	fibril_mutex_lock(&ring->guard);

	xhci_trb_t * const saved_enqueue_trb = ring->enqueue_trb;
	trb_segment_t * const saved_enqueue_segment = ring->enqueue_segment;
	if (phys)
		*phys = (uintptr_t)NULL;

	/*
	 * First, dry run and advance the enqueue pointer to see if the ring would
	 * be full anytime during the transaction.
	 */
	xhci_trb_t *trb = first_trb;
	for (size_t i = 0; i < trbs; ++i, ++trb) {
		if (phys && trb_generates_interrupt(trb)) {
			if (*phys) {
				err = ENOTSUP;
				goto err;
			}
			*phys = trb_ring_enqueue_phys(ring);
		}

		ring->enqueue_trb++;

		if (TRB_TYPE(*ring->enqueue_trb) == XHCI_TRB_TYPE_LINK)
			trb_ring_resolve_link(ring);

		if (trb_ring_enqueue_phys(ring) == ring->dequeue) {
			err = EAGAIN;
			goto err;
		}
	}

	ring->enqueue_segment = saved_enqueue_segment;
	ring->enqueue_trb = saved_enqueue_trb;

	/*
	 * Now, copy the TRBs without further checking.
	 */
	trb = first_trb;
	for (size_t i = 0; i < trbs; ++i, ++trb) {
		TRB_SET_CYCLE(*trb, ring->pcs);
		xhci_trb_copy_to_pio(ring->enqueue_trb, trb);

		usb_log_debug2("TRB ring(%p): Enqueued TRB %p", ring, trb);
		ring->enqueue_trb++;

		if (TRB_TYPE(*ring->enqueue_trb) == XHCI_TRB_TYPE_LINK) {
			TRB_SET_CYCLE(*ring->enqueue_trb, ring->pcs);

			if (TRB_LINK_TC(*ring->enqueue_trb)) {
				ring->pcs = !ring->pcs;
				usb_log_debug("TRB ring(%p): PCS toggled", ring);
			}

			trb_ring_resolve_link(ring);
		}
	}

	fibril_mutex_unlock(&ring->guard);
	return EOK;

err:
	ring->enqueue_segment = saved_enqueue_segment;
	ring->enqueue_trb = saved_enqueue_trb;
	fibril_mutex_unlock(&ring->guard);
	return err;
}

/**
 * Enqueue TD composed of a single TRB. See: `xhci_trb_ring_enqueue_multiple`
 */
errno_t xhci_trb_ring_enqueue(xhci_trb_ring_t *ring, xhci_trb_t *td, uintptr_t *phys)
{
	return xhci_trb_ring_enqueue_multiple(ring, td, 1, phys);
}

void xhci_trb_ring_reset_dequeue_state(xhci_trb_ring_t *ring, uintptr_t *addr)
{
	assert(ring);

	ring->dequeue = trb_ring_enqueue_phys(ring);

	if (addr)
		*addr = ring->dequeue | ring->pcs;
}

size_t xhci_trb_ring_size(xhci_trb_ring_t *ring)
{
	return ring->segment_count * SEGMENT_TRB_USEFUL_COUNT;
}

/**
 * Initializes an event ring.
 *
 * @param[in] initial_size A number of free slots on the ring, 0 leaves the
 * choice on a reasonable default (one page-sized segment).
 */
errno_t xhci_event_ring_init(xhci_event_ring_t *ring, size_t initial_size)
{
	errno_t err;
	if (initial_size == 0)
		initial_size = SEGMENT_TRB_COUNT;

	list_initialize(&ring->segments);

	size_t segment_count = (initial_size + SEGMENT_TRB_COUNT - 1) / SEGMENT_TRB_COUNT;
	size_t erst_size = segment_count * sizeof(xhci_erst_entry_t);

	if (dma_buffer_alloc(&ring->erst, erst_size)) {
		xhci_event_ring_fini(ring);
		return ENOMEM;
	}

	xhci_erst_entry_t *erst = ring->erst.virt;
	memset(erst, 0, erst_size);

	for (size_t i = 0; i < segment_count; i++) {
		trb_segment_t *segment;
		if ((err = trb_segment_alloc(&segment)) != EOK) {
			xhci_event_ring_fini(ring);
			return err;
		}

		list_append(&segment->segments_link, &ring->segments);
		ring->segment_count = i + 1;
		xhci_fill_erst_entry(&erst[i], segment->phys, SEGMENT_TRB_COUNT);
	}

	fibril_mutex_initialize(&ring->guard);

	usb_log_debug("Initialized event ring.");
	return EOK;
}

void xhci_event_ring_reset(xhci_event_ring_t *ring)
{
	list_foreach(ring->segments, segments_link, trb_segment_t, segment)
		memset(segment->trb_storage, 0, sizeof(segment->trb_storage));

	trb_segment_t * const segment = get_first_segment(&ring->segments);
	ring->dequeue_segment = segment;
	ring->dequeue_trb = segment_begin(segment);
	ring->dequeue_ptr = segment->phys;
	ring->ccs = 1;
}

void xhci_event_ring_fini(xhci_event_ring_t *ring)
{
	list_foreach_safe(ring->segments, cur, next) {
		trb_segment_t *segment = list_get_instance(cur, trb_segment_t, segments_link);
		trb_segment_free(segment);
	}

	dma_buffer_free(&ring->erst);
}

/**
 * Get the physical address of the dequeue pointer.
 */
static uintptr_t event_ring_dequeue_phys(xhci_event_ring_t *ring)
{
	uintptr_t trb_id = ring->dequeue_trb - segment_begin(ring->dequeue_segment);
	return ring->dequeue_segment->phys + trb_id * sizeof(xhci_trb_t);
}

/**
 * Fill the event with next valid event from the ring.
 *
 * @param event pointer to event to be overwritten
 * @return EOK on success,
 *         ENOENT when the ring is empty
 */
errno_t xhci_event_ring_dequeue(xhci_event_ring_t *ring, xhci_trb_t *event)
{
	fibril_mutex_lock(&ring->guard);

	/**
	 * The ERDP reported to the HC is a half-phase off the one we need to
	 * maintain. Therefore, we keep it extra.
	 */
	ring->dequeue_ptr = event_ring_dequeue_phys(ring);

	if (TRB_CYCLE(*ring->dequeue_trb) != ring->ccs) {
		fibril_mutex_unlock(&ring->guard);
		return ENOENT; /* The ring is empty. */
	}

	/* Do not reorder the Cycle bit reading with memcpy */
	read_barrier();

	memcpy(event, ring->dequeue_trb, sizeof(xhci_trb_t));

	ring->dequeue_trb++;
	const unsigned index = ring->dequeue_trb - segment_begin(ring->dequeue_segment);

	/* Wrapping around segment boundary */
	if (index >= SEGMENT_TRB_COUNT) {
		link_t *next_segment =
		    list_next(&ring->dequeue_segment->segments_link, &ring->segments);

		/* Wrapping around table boundary */
		if (!next_segment) {
			next_segment = list_first(&ring->segments);
			ring->ccs = !ring->ccs;
		}

		ring->dequeue_segment =
		    list_get_instance(next_segment, trb_segment_t, segments_link);
		ring->dequeue_trb = segment_begin(ring->dequeue_segment);
	}

	fibril_mutex_unlock(&ring->guard);
	return EOK;
}

void xhci_sw_ring_init(xhci_sw_ring_t *ring, size_t size)
{
	ring->begin = calloc(size, sizeof(xhci_trb_t));
	ring->end = ring->begin + size;

	fibril_mutex_initialize(&ring->guard);
	fibril_condvar_initialize(&ring->enqueued_cv);
	fibril_condvar_initialize(&ring->dequeued_cv);

	xhci_sw_ring_restart(ring);
}

errno_t xhci_sw_ring_enqueue(xhci_sw_ring_t *ring, xhci_trb_t *trb)
{
	assert(ring);
	assert(trb);

	fibril_mutex_lock(&ring->guard);
	while (ring->running && TRB_CYCLE(*ring->enqueue))
		fibril_condvar_wait(&ring->dequeued_cv, &ring->guard);

	*ring->enqueue = *trb;
	TRB_SET_CYCLE(*ring->enqueue, 1);
	if (++ring->enqueue == ring->end)
		ring->enqueue = ring->begin;
	fibril_condvar_signal(&ring->enqueued_cv);
	fibril_mutex_unlock(&ring->guard);

	return ring->running ? EOK : EINTR;
}

errno_t xhci_sw_ring_dequeue(xhci_sw_ring_t *ring, xhci_trb_t *trb)
{
	assert(ring);
	assert(trb);

	fibril_mutex_lock(&ring->guard);
	while (ring->running && !TRB_CYCLE(*ring->dequeue))
		fibril_condvar_wait(&ring->enqueued_cv, &ring->guard);

	*trb = *ring->dequeue;
	TRB_SET_CYCLE(*ring->dequeue, 0);
	if (++ring->dequeue == ring->end)
		ring->dequeue = ring->begin;
	fibril_condvar_signal(&ring->dequeued_cv);
	fibril_mutex_unlock(&ring->guard);

	return ring->running ? EOK : EINTR;
}

void xhci_sw_ring_stop(xhci_sw_ring_t *ring)
{
	ring->running = false;
	fibril_condvar_broadcast(&ring->enqueued_cv);
	fibril_condvar_broadcast(&ring->dequeued_cv);
}

void xhci_sw_ring_restart(xhci_sw_ring_t *ring)
{
	ring->enqueue = ring->dequeue = ring->begin;
	memset(ring->begin, 0, sizeof(xhci_trb_t) * (ring->end - ring->begin));
	ring->running = true;
}

void xhci_sw_ring_fini(xhci_sw_ring_t *ring)
{
	free(ring->begin);
}

/**
 * @}
 */
