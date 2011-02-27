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
 * Generic USB HID device structure and API.
 */

#ifndef USBHID_HIDDEV_H_
#define USBHID_HIDDEV_H_

#include <stdint.h>

#include <ddf/driver.h>

#include <usb/classes/hid.h>
#include <usb/pipes.h>
#include <usb/classes/hidparser.h>

/*----------------------------------------------------------------------------*/

/**
 * @brief USB/HID device type.
 */
typedef struct {
	ddf_dev_t *device;

	usb_device_connection_t wire;
	usb_endpoint_pipe_t ctrl_pipe;
	usb_endpoint_pipe_t poll_pipe;
	
	uint16_t iface;
	
	uint8_t *report_desc;
	usb_hid_report_parser_t *parser;
	
	int initialized;
} usbhid_dev_t;

/*----------------------------------------------------------------------------*/

usbhid_dev_t *usbhid_dev_new(void);

int usbhid_dev_init(usbhid_dev_t *hid_dev, ddf_dev_t *dev,
    usb_endpoint_description_t *poll_ep_desc);

/*----------------------------------------------------------------------------*/

#endif /* USBHID_HIDDEV_H_ */

/**
 * @}
 */
