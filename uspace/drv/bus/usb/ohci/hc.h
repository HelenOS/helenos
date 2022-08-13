/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI host controller driver structure
 */

#ifndef DRV_OHCI_HC_H
#define DRV_OHCI_HC_H

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

#include "ohci_regs.h"
#include "ohci_rh.h"
#include "ohci_bus.h"
#include "endpoint_list.h"
#include "hw_struct/hcca.h"

/** Main OHCI driver structure */
typedef struct hc {
	/** Common hcd header */
	hc_device_t base;

	/** Memory mapped I/O registers area */
	ohci_regs_t *registers;

	/** Host controller communication area structure */
	hcca_t *hcca;

	/** Transfer schedules */
	endpoint_list_t lists[4];

	/** List of active endpoints */
	list_t pending_endpoints;

	/** Guards schedule and endpoint manipulation */
	fibril_mutex_t guard;

	/** USB hub emulation structure */
	ohci_rh_t rh;

	/** USB bookkeeping */
	ohci_bus_t bus;
} hc_t;

static inline hc_t *hcd_to_hc(hc_device_t *hcd)
{
	assert(hcd);
	return (hc_t *) hcd;
}

extern errno_t hc_add(hc_device_t *, const hw_res_list_parsed_t *);
extern errno_t hc_gen_irq_code(irq_code_t *, hc_device_t *,
    const hw_res_list_parsed_t *, int *);
extern errno_t hc_gain_control(hc_device_t *);
extern errno_t hc_start(hc_device_t *);
extern errno_t hc_setup_roothub(hc_device_t *);
extern errno_t hc_gone(hc_device_t *);

extern void hc_enqueue_endpoint(hc_t *, const endpoint_t *);
extern void hc_dequeue_endpoint(hc_t *, const endpoint_t *);

extern errno_t ohci_hc_schedule(usb_transfer_batch_t *);
extern errno_t ohci_hc_status(bus_t *, uint32_t *);
extern void ohci_hc_interrupt(bus_t *, uint32_t);

#endif

/**
 * @}
 */
