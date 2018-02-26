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
