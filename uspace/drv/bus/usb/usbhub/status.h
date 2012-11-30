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

#include <stdbool.h>
#include <sys/types.h>
#include <usb/dev/request.h>

/**
 * structure holding port status and changes flags.
 * should not be accessed directly, use supplied getter/setter methods.
 *
 * For more information refer to tables 11-15 and 11-16 in
 * "Universal Serial Bus Specification Revision 1.1" pages 274 and 277
 * (290 and 293 in pdf)
 *
 */
typedef uint32_t usb_port_status_t;
#define USB_HUB_PORT_STATUS_CONNECTION \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_PORT_CONNECTION)))
#define USB_HUB_PORT_STATUS_ENABLED \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_PORT_ENABLE)))
#define USB_HUB_PORT_STATUS_SUSPEND \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_PORT_SUSPEND)))
#define USB_HUB_PORT_STATUS_OC \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_PORT_OVER_CURRENT)))
#define USB_HUB_PORT_STATUS_RESET \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_PORT_RESET)))
#define USB_HUB_PORT_STATUS_POWER \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_PORT_POWER)))
#define USB_HUB_PORT_STATUS_LOW_SPEED \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_PORT_LOW_SPEED)))
#define USB_HUB_PORT_STATUS_HIGH_SPEED \
    (uint32_usb2host(1 << 10))

#define USB_HUB_PORT_C_STATUS_CONNECTION \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_C_PORT_CONNECTION)))
#define USB_HUB_PORT_C_STATUS_ENABLED \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_C_PORT_ENABLE)))
#define USB_HUB_PORT_C_STATUS_SUSPEND \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_C_PORT_SUSPEND)))
#define USB_HUB_PORT_C_STATUS_OC \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_C_PORT_OVER_CURRENT)))
#define USB_HUB_PORT_C_STATUS_RESET \
    (uint32_usb2host(1 << (USB_HUB_FEATURE_C_PORT_RESET)))

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


/**
 * speed getter for port status
 *
 * @param status
 * @return speed of usb device (for more see usb specification)
 */
static inline usb_speed_t usb_port_speed(usb_port_status_t status)
{
	if ((status & USB_HUB_PORT_STATUS_LOW_SPEED) != 0)
		return USB_SPEED_LOW;
	if ((status & USB_HUB_PORT_STATUS_HIGH_SPEED) != 0)
		return USB_SPEED_HIGH;
	return USB_SPEED_FULL;
}

#endif	/* HUB_STATUS_H */
/**
 * @}
 */
