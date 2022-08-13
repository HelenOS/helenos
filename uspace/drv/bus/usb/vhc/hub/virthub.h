/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
