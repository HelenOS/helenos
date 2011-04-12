/*
 * Copyright (c) 2011 Lubos Slovak
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

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * USB HID subdriver mappings.
 */

#ifndef USB_HID_SUBDRIVERS_H_
#define USB_HID_SUBDRIVERS_H_

#include "usbhid.h"
#include "kbd/kbddev.h"

/*----------------------------------------------------------------------------*/

typedef struct usb_hid_subdriver_usage {
	int usage_page;
	int usage;
} usb_hid_subdriver_usage_t;

/*----------------------------------------------------------------------------*/

/* TODO: This mapping must contain some other information to get the proper
 *       interface.
 */
typedef struct usb_hid_subdriver_mapping {
	const usb_hid_subdriver_usage_t *usage_path;
	int compare;
	uint16_t vendor_id;
	uint16_t product_id;
	usb_hid_subdriver_t subdriver;
} usb_hid_subdriver_mapping_t;

/*----------------------------------------------------------------------------*/

extern const usb_hid_subdriver_mapping_t usb_hid_subdrivers[];

/*----------------------------------------------------------------------------*/

#endif /* USB_HID_SUBDRIVERS_H_ */

/**
 * @}
 */
