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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief USB hub related structures.
 */
#ifndef LIBUSB_HUB_H_
#define LIBUSB_HUB_H_

/** Hub class request. */
typedef enum {
	USB_HUB_REQUEST_GET_STATUS = 0,
	USB_HUB_REQUEST_CLEAR_FEATURE = 1,
	USB_HUB_REQUEST_GET_STATE = 2,
	USB_HUB_REQUEST_SET_FEATURE = 3,
	USB_HUB_REQUEST_GET_DESCRIPTOR = 6,
	USB_HUB_REQUEST_SET_DESCRIPTOR = 7,
	/* USB_HUB_REQUEST_ = , */
} usb_hub_class_request_t;

/** Hub class feature selector.
 * @warning The constants are not unique (feature selectors are used
 * for hub and port).
 */
typedef enum {
	USB_HUB_FEATURE_C_HUB_LOCAL_POWER = 0,
	USB_HUB_FEATURE_C_HUB_OVER_CURRENT = 1,
	USB_HUB_FEATURE_PORT_CONNECTION = 0,
	USB_HUB_FEATURE_PORT_ENABLE = 1,
	USB_HUB_FEATURE_PORT_SUSPEND = 2,
	USB_HUB_FEATURE_PORT_OVER_CURRENT = 3,
	USB_HUB_FEATURE_PORT_RESET = 4,
	USB_HUB_FEATURE_PORT_POWER = 8,
	USB_HUB_FEATURE_PORT_LOW_SPEED = 9,
	USB_HUB_FEATURE_C_PORT_CONNECTION = 16,
	USB_HUB_FEATURE_C_PORT_ENABLE = 17,
	USB_HUB_FEATURE_C_PORT_SUSPEND = 18,
	USB_HUB_FEATURE_C_PORT_OVER_CURRENT = 19,
	USB_HUB_FEATURE_C_PORT_RESET = 20,
	/* USB_HUB_FEATURE_ = , */
} usb_hub_class_feature_t;

#endif
/**
 * @}
 */
