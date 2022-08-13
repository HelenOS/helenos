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
 */
#ifndef XHCI_DEVICE_H
#define XHCI_DEVICE_H

#include <usb/host/bus.h>
#include <usb/dma_buffer.h>

typedef struct xhci_slot_ctx xhci_slot_ctx_t;

typedef struct xhci_device {
	device_t base;		/**< Inheritance. Keep this first. */

	/** Slot ID assigned to the device by xHC. */
	uint32_t slot_id;

	/** Corresponding port on RH */
	uint8_t rh_port;

	/** Route string */
	uint32_t route_str;

	/** Place to store the allocated context */
	dma_buffer_t dev_ctx;

	/** Hub specific information. Valid only if the device is_hub. */
	bool is_hub;
	uint8_t num_ports;
	uint8_t tt_think_time;
} xhci_device_t;

#define XHCI_DEV_FMT  "(%s, slot %d)"
#define XHCI_DEV_ARGS(dev)	ddf_fun_get_name((dev).base.fun), (dev).slot_id

/* Bus callbacks */
errno_t xhci_device_enumerate(device_t *);
void xhci_device_offline(device_t *);
errno_t xhci_device_online(device_t *);
void xhci_device_gone(device_t *);

void xhci_setup_slot_context(xhci_device_t *, xhci_slot_ctx_t *);

static inline xhci_device_t *xhci_device_get(device_t *dev)
{
	assert(dev);
	return (xhci_device_t *) dev;
}

#endif
/**
 * @}
 */
