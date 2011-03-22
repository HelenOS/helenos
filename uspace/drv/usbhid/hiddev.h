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
 *
 * @todo Add function for parsing report - this is generic HID function, not
 *       keyboard-specific, as the report parser is also generic.
 * @todo Add function for polling as that is also a generic HID process.
 * @todo Add interrupt in pipe to the structure.
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
 * USB/HID device type.
 *
 * Holds a reference to DDF device structure, and HID-specific data, such 
 * as information about used pipes (one Control pipe and one Interrupt In pipe),
 * polling interval, assigned interface number, Report descriptor and a
 * reference to the Report parser used to parse incoming reports and composing
 * outgoing reports.
 */
typedef struct {
	/** DDF device representing the controlled HID device. */
	ddf_dev_t *device;

	/** Physical connection to the device. */
	usb_device_connection_t wire;
	/** USB pipe corresponding to the default Control endpoint. */
	usb_pipe_t ctrl_pipe;
	/** USB pipe corresponding to the Interrupt In (polling) pipe. */
	usb_pipe_t poll_pipe;
	
	/** Polling interval retreived from the Interface descriptor. */
	short poll_interval;
	
	/** Interface number assigned to this device. */
	uint16_t iface;
	
	/** Report descriptor. */
	uint8_t *report_desc;

	size_t report_desc_size;

	/** HID Report parser. */
	usb_hid_report_parser_t *parser;
	
	/** State of the structure (for checking before use). */
	int initialized;
} usbhid_dev_t;

/*----------------------------------------------------------------------------*/

usbhid_dev_t *usbhid_dev_new(void);

void usbhid_dev_free(usbhid_dev_t **hid_dev);

int usbhid_dev_init(usbhid_dev_t *hid_dev, ddf_dev_t *dev,
    usb_endpoint_description_t *poll_ep_desc);

/*----------------------------------------------------------------------------*/

#endif /* USBHID_HIDDEV_H_ */

/**
 * @}
 */
