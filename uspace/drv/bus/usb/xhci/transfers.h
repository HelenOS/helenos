/*
 * SPDX-FileCopyrightText: 2018 Michal Staruch, Petr Manek, Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller transfer ring management
 */

#ifndef XHCI_TRANSFERS_H
#define XHCI_TRANSFERS_H

#include <usb/host/usb_transfer_batch.h>

#include "hw_struct/context.h"
#include "trb_ring.h"

typedef struct xhci_hc xhci_hc_t;

typedef struct {
	usb_transfer_batch_t batch;
	link_t link;

	uint8_t direction;

	uintptr_t interrupt_trb_phys;
} xhci_transfer_t;

extern usb_transfer_batch_t *xhci_transfer_create(endpoint_t *);
extern errno_t xhci_transfer_schedule(usb_transfer_batch_t *);

extern errno_t xhci_handle_transfer_event(xhci_hc_t *, xhci_trb_t *);
extern void xhci_transfer_destroy(usb_transfer_batch_t *);

static inline xhci_transfer_t *xhci_transfer_from_batch(
    usb_transfer_batch_t *batch)
{
	assert(batch);
	return (xhci_transfer_t *) batch;
}

/**
 * @}
 */
#endif
