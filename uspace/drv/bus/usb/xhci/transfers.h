/*
 * Copyright (c) 2018 Michal Staruch, Petr Manek, Ondrej Hlavaty
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

extern usb_transfer_batch_t* xhci_transfer_create(endpoint_t *);
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
