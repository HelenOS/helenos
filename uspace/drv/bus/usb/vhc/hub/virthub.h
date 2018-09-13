/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * @brief USB hub as a virtual USB device.
 */

#ifndef VHC_HUB_VIRTHUB_H_
#define VHC_HUB_VIRTHUB_H_

#include <usbvirt/device.h>
#include "hub.h"

#ifdef STANDALONE_HUB
#define virtdev_connection_t int
#else
#include "../vhcd.h"
#endif

/** Endpoint number for status change pipe. */
#define HUB_STATUS_CHANGE_PIPE 1
/** Configuration value for hub configuration. */
#define HUB_CONFIGURATION_ID 1

/** Hub descriptor.
 */
typedef struct {
	/** Size of this descriptor in bytes. */
	uint8_t length;
	/** Descriptor type (USB_DESCTYPE_HUB). */
	uint8_t type;
	/** Number of downstream ports. */
	uint8_t port_count;
	/** Hub characteristics. */
	uint16_t characteristics;
	/** Time from power-on to stabilized current.
	 * Expressed in 2ms unit.
	 */
	uint8_t power_on_warm_up;
	/** Maximum current (in mA). */
	uint8_t max_current;
	/** Whether device at given port is removable. */
	uint8_t removable_device[BITS2BYTES(HUB_PORT_COUNT + 1)];
	/** Port power control.
	 * This is USB1.0 compatibility field, all bits must be 1.
	 */
	uint8_t port_power[BITS2BYTES(HUB_PORT_COUNT + 1)];
} __attribute__((packed)) hub_descriptor_t;

extern usbvirt_device_ops_t hub_ops;
extern hub_descriptor_t hub_descriptor;

errno_t virthub_init(usbvirt_device_t *, const char *name);
int virthub_connect_device(usbvirt_device_t *, vhc_virtdev_t *);
errno_t virthub_disconnect_device(usbvirt_device_t *, vhc_virtdev_t *);
bool virthub_is_device_enabled(usbvirt_device_t *, vhc_virtdev_t *);
void virthub_get_status(usbvirt_device_t *, char *, size_t);

#endif

/**
 * @}
 */
