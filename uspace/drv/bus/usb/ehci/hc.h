/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI host controller driver structure
 */
#ifndef DRV_EHCI_HC_H
#define DRV_EHCI_HC_H

#include <adt/list.h>
#include <ddi.h>
#include <ddf/driver.h>
#include <device/hw_res_parsed.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stdint.h>

#include <usb/host/hcd.h>
#include <usb/host/endpoint.h>
#include <usb/host/usb_transfer_batch.h>

#include "ehci_regs.h"
#include "ehci_rh.h"
#include "hw_struct/link_pointer.h"
#include "endpoint_list.h"

/** Main EHCI driver structure */
typedef struct hc {
	/* Common device header */
	hc_device_t base;

	/** Memory mapped CAPS register area */
	ehci_caps_regs_t *caps;
	/** Memory mapped I/O registers area */
	ehci_regs_t *registers;

	/** Iso transfer list, backed by dma_buffer */
	link_pointer_t *periodic_list;

	dma_buffer_t dma_buffer;

	/** CONTROL and BULK schedules */
	endpoint_list_t async_list;

	/** INT schedule */
	endpoint_list_t int_list;

	/** List of active transfers */
	list_t pending_endpoints;

	/** Guards schedule and endpoint manipulation */
	fibril_mutex_t guard;

	/** Wait for hc to restart async chedule */
	fibril_condvar_t async_doorbell;

	/** USB hub emulation structure */
	ehci_rh_t rh;

	/** USB bookkeeping */
	ehci_bus_t bus;
} hc_t;

static inline hc_t *hcd_to_hc(hc_device_t *hcd)
{
	assert(hcd);
	return (hc_t *) hcd;
}

void hc_enqueue_endpoint(hc_t *, const endpoint_t *);
void hc_dequeue_endpoint(hc_t *, const endpoint_t *);

/* Boottime operations */
extern errno_t hc_add(hc_device_t *, const hw_res_list_parsed_t *);
extern errno_t hc_start(hc_device_t *);
extern errno_t hc_setup_roothub(hc_device_t *);
extern errno_t hc_gen_irq_code(irq_code_t *, hc_device_t *,
    const hw_res_list_parsed_t *, int *);
extern errno_t hc_gone(hc_device_t *);

/** Runtime operations */
extern void ehci_hc_interrupt(bus_t *, uint32_t);
extern errno_t ehci_hc_status(bus_t *, uint32_t *);
extern errno_t ehci_hc_schedule(usb_transfer_batch_t *);

#endif
/**
 * @}
 */
