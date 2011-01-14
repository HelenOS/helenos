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
 * @brief USB HID device related types.
 */
#ifndef LIBUSB_HID_H_
#define LIBUSB_HID_H_

#include <usb/usb.h>
#include <driver.h>
#include <usb/classes/hidparser.h>
#include <usb/descriptor.h>

/** USB/HID device requests. */
typedef enum {
	USB_HIDREQ_GET_REPORT = 1,
	USB_HIDREQ_GET_IDLE = 2,
	USB_HIDREQ_GET_PROTOCOL = 3,
	/* Values 4 to 8 are reserved. */
	USB_HIDREQ_SET_REPORT = 9,
	USB_HIDREQ_SET_IDLE = 10,
	USB_HIDREQ_SET_PROTOCOL = 11
} usb_hid_request_t;

/** USB/HID interface protocols. */
typedef enum {
	USB_HID_PROTOCOL_NONE = 0,
	USB_HID_PROTOCOL_KEYBOARD = 1,
	USB_HID_PROTOCOL_MOUSE = 2
} usb_hid_protocol_t;

/** Part of standard USB HID descriptor specifying one class descriptor.
 *
 * (See HID Specification, p.22)
 */
typedef struct {
	/** Type of class-specific descriptor (Report or Physical). */
	uint8_t type;
	/** Length of class-specific descriptor in bytes. */
	uint16_t length;
} __attribute__ ((packed)) usb_standard_hid_class_descriptor_info_t;

/** Standard USB HID descriptor.
 *
 * (See HID Specification, p.22)
 * 
 * It is actually only the "header" of the descriptor, it does not contain
 * the last two mandatory fields (type and length of the first class-specific
 * descriptor).
 */
typedef struct {
	/** Total size of this descriptor in bytes. 
	 *
	 * This includes all class-specific descriptor info - type + length 
	 * for each descriptor.
	 */
	uint8_t length;
	/** Descriptor type (USB_DESCTYPE_HID). */
	uint8_t descriptor_type;
	/** HID Class Specification release. */
	uint16_t spec_release;
	/** Country code of localized hardware. */
	uint8_t country_code;
	/** Total number of class-specific (i.e. Report and Physical) 
	 * descriptors. 
	 *
	 * @note There is always only one Report descriptor.
	 */
	uint8_t class_desc_count;
	/** First mandatory class descriptor (Report) info. */
	usb_standard_hid_descriptor_class_item_t report_desc_info;
} __attribute__ ((packed)) usb_standard_hid_descriptor_t;

/**
 *
 */
typedef struct {
	usb_standard_interface_descriptor_t iface_desc;
	usb_standard_endpoint_descriptor_t *endpoints;
	usb_standard_hid_descriptor_t hid_desc;
	uint8_t *report_desc;
	//usb_standard_hid_class_descriptor_info_t *class_desc_info;
	//uint8_t **class_descs;
} usb_hid_iface_t;

/**
 *
 */
typedef struct {
	usb_standard_configuration_descriptor_t config_descriptor;
	usb_hid_iface_t *interfaces;
} usb_hid_configuration_t;

/**
 * @brief USB/HID keyboard device type.
 *
 * Quite dummy right now.
 */
typedef struct {
	device_t *device;
	usb_hid_configuration_t *conf;
	usb_address_t address;
	usb_endpoint_t poll_endpoint;
	usb_hid_report_parser_t *parser;
} usb_hid_dev_kbd_t;

// TODO: more configurations!

#endif
/**
 * @}
 */
