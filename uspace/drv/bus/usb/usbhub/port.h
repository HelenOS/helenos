/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * Hub port handling.
 */

#ifndef DRV_USBHUB_PORT_H
#define DRV_USBHUB_PORT_H

#include <usb/dev/driver.h>
#include <usb/classes/hub.h>
#include <usb/port.h>

typedef struct usb_hub_dev usb_hub_dev_t;

/** Information about single port on a hub. */
typedef struct {
	usb_port_t base;
	/* Parenting hub */
	usb_hub_dev_t *hub;
	/** Port number as reported in descriptors. */
	unsigned int port_number;
	/** Speed at the time of enabling the port */
	usb_speed_t speed;
} usb_hub_port_t;

void usb_hub_port_init(usb_hub_port_t *, usb_hub_dev_t *, unsigned int);

void usb_hub_port_process_interrupt(usb_hub_port_t *port);

#endif

/**
 * @}
 */
