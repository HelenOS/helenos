/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 * Bus implementation common for OHCI, UHCI and EHCI.
 */
#ifndef LIBUSBHOST_HOST_USB2_BUS_H
#define LIBUSBHOST_HOST_USB2_BUS_H

#include <adt/list.h>
#include <stdbool.h>
#include <usb/usb.h>

#include <usb/host/bus.h>
#include <usb/host/bandwidth.h>

typedef struct usb2_bus usb2_bus_t;
typedef struct endpoint endpoint_t;

/** Endpoint and bandwidth management structure */
typedef struct usb2_bus_helper {
	/** Map of occupied addresses */
	bool address_occupied [USB_ADDRESS_COUNT];
	/** The last reserved address */
	usb_address_t last_address;

	/** Size of the bandwidth pool */
	size_t free_bw;

	/* Configured bandwidth accounting */
	const bandwidth_accounting_t *bw_accounting;
} usb2_bus_helper_t;

extern void usb2_bus_helper_init(usb2_bus_helper_t *,
    const bandwidth_accounting_t *);

extern int usb2_bus_device_enumerate(usb2_bus_helper_t *, device_t *);
extern void usb2_bus_device_gone(usb2_bus_helper_t *, device_t *);

extern int usb2_bus_endpoint_register(usb2_bus_helper_t *, endpoint_t *);
extern void usb2_bus_endpoint_unregister(usb2_bus_helper_t *, endpoint_t *);

#endif
/**
 * @}
 */
