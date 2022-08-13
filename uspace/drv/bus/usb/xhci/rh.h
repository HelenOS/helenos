/*
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Petr Manek, Michal Staruch
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The roothub structures abstraction.
 */

#ifndef XHCI_RH_H
#define XHCI_RH_H

#include <usb/host/bus.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/host/utility.h>

#include "hw_struct/regs.h"
#include "endpoint.h"

typedef struct xhci_hc xhci_hc_t;
typedef struct ddf_dev ddf_dev_t;

/**
 * xHCI lets the controller define speeds of ports it controls.
 */
typedef struct xhci_port_speed {
	char name [4];
	uint8_t major, minor;
	uint64_t rx_bps, tx_bps;
	usb_speed_t usb_speed;
} xhci_port_speed_t;

typedef struct hcd_roothub hcd_roothub_t;
typedef struct xhci_bus xhci_bus_t;
typedef struct rh_port rh_port_t;

/* XHCI root hub instance */
typedef struct {
	/** Host controller */
	xhci_hc_t *hc;

	/* Root for the device tree */
	xhci_device_t device;

	/* Number of hub ports. */
	size_t max_ports;

	/* Array of port structures. (size is `max_ports`) */
	rh_port_t *ports;

	/* Event ring for roothub */
	xhci_sw_ring_t event_ring;

	joinable_fibril_t *event_worker;
} xhci_rh_t;

extern errno_t xhci_rh_init(xhci_rh_t *, xhci_hc_t *);
extern errno_t xhci_rh_fini(xhci_rh_t *);

extern void xhci_rh_set_ports_protocol(xhci_rh_t *, unsigned, unsigned, unsigned);
extern void xhci_rh_start(xhci_rh_t *);
extern void xhci_rh_stop(xhci_rh_t *rh);

#endif

/**
 * @}
 */
