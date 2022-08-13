/*
 * SPDX-FileCopyrightText: 2010 Matus Dekanek
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhub
 * @{
 */

#ifndef HUB_STATUS_H
#define	HUB_STATUS_H

#include <usb/dev/request.h>

/**
 * structure holding hub status and changes flags.
 *
 * For more information refer to table 11.16.2.5 in
 * "Universal Serial Bus Specification Revision 1.1"
 *
 */
typedef uint32_t usb_hub_status_t;
#define USB_HUB_STATUS_OVER_CURRENT \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_HUB_OVER_CURRENT)))
#define USB_HUB_STATUS_LOCAL_POWER \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_HUB_LOCAL_POWER)))

#define USB_HUB_STATUS_C_OVER_CURRENT \
    (uint32_usb2host(1 << (16 + USB_HUB_FEATURE_C_HUB_OVER_CURRENT)))
#define USB_HUB_STATUS_C_LOCAL_POWER \
    (uint32_usb2host(1 << (16 + USB_HUB_FEATURE_C_HUB_LOCAL_POWER)))

static inline usb_speed_t usb_port_speed(usb_speed_t hub_speed, uint32_t status)
{
	if (hub_speed == USB_SPEED_SUPER)
		return USB_SPEED_SUPER;
	if (hub_speed == USB_SPEED_HIGH &&
	    (status & USB2_HUB_PORT_STATUS_HIGH_SPEED))
		return USB_SPEED_HIGH;
	if ((status & USB2_HUB_PORT_STATUS_LOW_SPEED) != 0)
		return USB_SPEED_LOW;
	return USB_SPEED_FULL;
}

#endif

/**
 * @}
 */
