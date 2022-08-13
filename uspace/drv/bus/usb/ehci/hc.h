/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
