/*
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief Data structures and functions related to isochronous transfers.
 */

#ifndef XHCI_ISOCH_H
#define XHCI_ISOCH_H

#include <usb/dma_buffer.h>

#include "trb_ring.h"

#include "transfers.h"

typedef struct xhci_endpoint xhci_endpoint_t;

typedef enum {
	ISOCH_EMPTY,	/* Unused yet */
	ISOCH_FILLED,	/* The data buffer is valid */
	ISOCH_FED,	/* The data buffer is in possession of xHC */
	ISOCH_COMPLETE,	/* The error code is valid */
} xhci_isoch_transfer_state_t;

typedef struct {
	/** Buffer with data */
	dma_buffer_t data;
	/** Used buffer size */
	size_t size;

	/** Current state */
	xhci_isoch_transfer_state_t state;

	/** Microframe to which to schedule */
	uint64_t mfindex;

	/** Physical address of enqueued TRB */
	uintptr_t interrupt_trb_phys;

	/** Result of the transfer. Valid only if status == ISOCH_COMPLETE. */
	errno_t error;
} xhci_isoch_transfer_t;

typedef struct {
	/** Protects common buffers. */
	fibril_mutex_t guard;

	/** Signals filled buffer. */
	fibril_condvar_t avail;

	/** Defers handing buffers to the HC. */
	fibril_timer_t *feeding_timer;

	/** Resets endpoint if there is no traffic. */
	fibril_timer_t *reset_timer;

	/**
	 * The maximum size of an isochronous transfer
	 * and therefore the size of buffers
	 */
	size_t max_size;

	/** The microframe at which the last TRB was scheduled. */
	uint64_t last_mf;

	/** The number of transfer buffers allocated */
	size_t buffer_count;

	/** Isochronous scheduled transfers with respective buffers */
	xhci_isoch_transfer_t *transfers;

	/**
	 * Out: Next buffer that will be handed to HW.
	 * In:  Invalid. Hidden inside HC.
	 */
	size_t hw_enqueue;

	/**
	 * Out: Next buffer that will be used for writing.
	 * In:  Next buffer that will be enqueued to be written by the HC
	 */
	size_t enqueue;

	/**
	 * Out: First buffer that will be checked for completion
	 * In:  Next buffer to be read from, when valid.
	 */
	size_t dequeue;
} xhci_isoch_t;

typedef struct usb_endpoint_descriptors usb_endpoint_descriptors_t;

extern void isoch_init(xhci_endpoint_t *, const usb_endpoint_descriptors_t *);
extern void isoch_fini(xhci_endpoint_t *);
extern errno_t isoch_alloc_transfers(xhci_endpoint_t *);

extern errno_t isoch_schedule_out(xhci_transfer_t *);
extern errno_t isoch_schedule_in(xhci_transfer_t *);
extern void isoch_handle_transfer_event(xhci_hc_t *, xhci_endpoint_t *,
    xhci_trb_t *);

#endif

/**
 * @}
 */
