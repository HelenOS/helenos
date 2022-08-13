/*
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 * An implementation of bus keeper for xHCI. The (physical) HC itself takes
 * care about addressing devices, so this implementation is actually simpler
 * than those of [OUE]HCI.
 */
#ifndef XHCI_BUS_H
#define XHCI_BUS_H

#include <usb/host/bus.h>

typedef struct xhci_hc xhci_hc_t;
typedef struct xhci_device xhci_device_t;

/** Endpoint management structure */
typedef struct xhci_bus {
	bus_t base; /**< Inheritance. Keep this first. */
	xhci_hc_t *hc; /**< Pointer to managing HC (to issue commands) */
	xhci_device_t **devices_by_slot; /**< Devices by Slot ID */
} xhci_bus_t;

extern errno_t xhci_bus_init(xhci_bus_t *, xhci_hc_t *);
extern void xhci_bus_fini(xhci_bus_t *);

static inline xhci_bus_t *bus_to_xhci_bus(bus_t *bus_base)
{
	assert(bus_base);
	return (xhci_bus_t *) bus_base;
}

#endif
/**
 * @}
 */
