/*
 * Copyright (c) 2018 Ondrej Hlavaty, Jan Hrach
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * TRB Ring is a data structure for communication between HC and software.
 *
 * Despite this description, it is not used as an hardware structure - all but
 * the Event ring is used as buffer of the TRBs itself, linked by Link TRB to
 * form a (possibly multi-segment) circular buffer.
 *
 * This data structure abstracts this behavior.
 */

#ifndef XHCI_TRB_RING_H
#define XHCI_TRB_RING_H

#include <adt/list.h>
#include <fibril_synch.h>
#include <libarch/config.h>
#include <usb/dma_buffer.h>

typedef struct trb_segment trb_segment_t;
typedef struct xhci_hc xhci_hc_t;
typedef struct xhci_trb xhci_trb_t;
typedef struct xhci_erst_entry xhci_erst_entry_t;

/**
 * A TRB ring of which the software is a producer - command / transfer.
 */
typedef struct xhci_trb_ring {
	list_t segments;                /**< List of assigned segments */
	int segment_count;              /**< Number of segments assigned */

	/**
	 * As the link TRBs connect physical addresses, we need to keep track
	 * of active segment in virtual memory. The enqueue ptr should always
	 * belong to the enqueue segment.
	 */
	trb_segment_t *enqueue_segment;
	xhci_trb_t *enqueue_trb;

	uintptr_t dequeue; /**< Last reported position of the dequeue pointer */
	bool pcs;          /**< Producer Cycle State: section 4.9.2 */

	fibril_mutex_t guard;
} xhci_trb_ring_t;

extern errno_t xhci_trb_ring_init(xhci_trb_ring_t *, size_t);
extern void xhci_trb_ring_fini(xhci_trb_ring_t *);
extern errno_t xhci_trb_ring_enqueue(xhci_trb_ring_t *, xhci_trb_t *,
    uintptr_t *);
extern errno_t xhci_trb_ring_enqueue_multiple(xhci_trb_ring_t *, xhci_trb_t *,
    size_t, uintptr_t *);
extern size_t xhci_trb_ring_size(xhci_trb_ring_t *);

extern void xhci_trb_ring_reset_dequeue_state(xhci_trb_ring_t *, uintptr_t *);

/**
 * When an event is received by the upper layer, it needs to update the dequeue
 * pointer inside the ring. Otherwise, the ring will soon show up as full.
 */
static inline void xhci_trb_ring_update_dequeue(xhci_trb_ring_t *ring,
    uintptr_t phys)
{
	ring->dequeue = phys;
}

/**
 * A TRB ring of which the software is a consumer (event rings).
 */
typedef struct xhci_event_ring {
	list_t segments;                /**< List of assigned segments */
	int segment_count;              /**< Number of segments assigned */

	trb_segment_t *dequeue_segment; /**< Current segment */
	xhci_trb_t *dequeue_trb;        /**< Next TRB to be processed */
	uintptr_t dequeue_ptr;  /**< Physical ERDP to be reported to the HC */

	dma_buffer_t erst;      /**< ERST given to the HC */

	bool ccs;               /**< Consumer Cycle State: section 4.9.2 */

	fibril_mutex_t guard;
} xhci_event_ring_t;

extern errno_t xhci_event_ring_init(xhci_event_ring_t *, size_t);
extern void xhci_event_ring_fini(xhci_event_ring_t *);
extern void xhci_event_ring_reset(xhci_event_ring_t *);
extern errno_t xhci_event_ring_dequeue(xhci_event_ring_t *, xhci_trb_t *);

/**
 * A TRB ring of which the software is both consumer and provider.
 */
typedef struct xhci_sw_ring {
	xhci_trb_t *begin, *end;
	xhci_trb_t *enqueue, *dequeue;

	fibril_mutex_t guard;
	fibril_condvar_t enqueued_cv, dequeued_cv;

	bool running;
} xhci_sw_ring_t;

extern void xhci_sw_ring_init(xhci_sw_ring_t *, size_t);

/* Both may block if the ring is full/empty. */
extern errno_t xhci_sw_ring_enqueue(xhci_sw_ring_t *, xhci_trb_t *);
extern errno_t xhci_sw_ring_dequeue(xhci_sw_ring_t *, xhci_trb_t *);

extern void xhci_sw_ring_restart(xhci_sw_ring_t *);
extern void xhci_sw_ring_stop(xhci_sw_ring_t *);
extern void xhci_sw_ring_fini(xhci_sw_ring_t *);

#endif

/**
 * @}
 */
