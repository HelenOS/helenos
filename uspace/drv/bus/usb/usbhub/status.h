/*
 * Copyright (c) 2010 Matus Dekanek
 * Copyright (c) 2011 Jan Vesely
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
