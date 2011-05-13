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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Standard USB descriptors.
 */
#ifndef LIBUSB_DESCRIPTOR_H_
#define LIBUSB_DESCRIPTOR_H_

#include <async.h>

/** Descriptor type. */
typedef enum {
	USB_DESCTYPE_DEVICE = 1,
	USB_DESCTYPE_CONFIGURATION = 2,
	USB_DESCTYPE_STRING = 3,
	USB_DESCTYPE_INTERFACE = 4,
	USB_DESCTYPE_ENDPOINT = 5,
	USB_DESCTYPE_HID = 0x21,
	USB_DESCTYPE_HID_REPORT = 0x22,
	USB_DESCTYPE_HID_PHYSICAL = 0x23,
	USB_DESCTYPE_HUB = 0x29,
	/* USB_DESCTYPE_ = */
} usb_descriptor_type_t;

/** Standard USB device descriptor.
 */
typedef struct {
	/** Size of this descriptor in bytes. */
	uint8_t length;
	/** Descriptor type (USB_DESCTYPE_DEVICE). */
	uint8_t descriptor_type;
	/** USB specification release number.
	 * The number shall be coded as binary-coded decimal (BCD).
	 */
	uint16_t usb_spec_version;
	/** Device class. */
	uint8_t device_class;
	/** Device sub-class. */
	uint8_t device_subclass;
	/** Device protocol. */
	uint8_t device_protocol;
	/** Maximum packet size for endpoint zero.
	 * Valid values are only 8, 16, 32, 64).
	 */
	uint8_t max_packet_size;
	/** Vendor ID. */
	uint16_t vendor_id;
	/** Product ID. */
	uint16_t product_id;
	/** Device release number (in BCD). */
	uint16_t device_version;
	/** Manufacturer descriptor index. */
	uint8_t str_manufacturer;
	/** Product descriptor index. */
	uint8_t str_product;
	/** Device serial number descriptor index. */
	uint8_t str_serial_number;
	/** Number of possible configurations. */
	uint8_t configuration_count;
} __attribute__ ((packed)) usb_standard_device_descriptor_t;

/** Standard USB configuration descriptor.
 */
typedef struct {
	/** Size of this descriptor in bytes. */
	uint8_t length;
	/** Descriptor type (USB_DESCTYPE_CONFIGURATION). */
	uint8_t descriptor_type;
	/** Total length of all data of this configuration.
	 * This includes the combined length of all descriptors
	 * (configuration, interface, endpoint, class-specific and
	 * vendor-specific) valid for this configuration.
	 */
	uint16_t total_length;
	/** Number of possible interfaces under this configuration. */
	uint8_t interface_count;
	/** Configuration value used when setting this configuration. */
	uint8_t configuration_number;
	/** String descriptor describing this configuration. */
	uint8_t str_configuration;
	/** Attribute bitmap. */
	uint8_t attributes;
	/** Maximum power consumption from the USB under this configuration.
	 * Expressed in 2mA unit (e.g. 50 ~ 100mA).
	 */
	uint8_t max_power;
} __attribute__ ((packed)) usb_standard_configuration_descriptor_t;

/** Standard USB interface descriptor.
 */
typedef struct {
	/** Size of this descriptor in bytes. */
	uint8_t length;
	/** Descriptor type (USB_DESCTYPE_INTERFACE). */
	uint8_t descriptor_type;
	/** Number of interface.
	 * Zero-based index into array of interfaces for current configuration.
	 */
	uint8_t interface_number;
	/** Alternate setting for value in interface_number. */
	uint8_t alternate_setting;
	/** Number of endpoints used by this interface.
	 * This number must exclude usage of endpoint zero
	 * (default control pipe).
	 */
	uint8_t endpoint_count;
	/** Class code. */
	uint8_t interface_class;
	/** Subclass code. */
	uint8_t interface_subclass;
	/** Protocol code. */
	uint8_t interface_protocol;
	/** String descriptor describing this interface. */
	uint8_t str_interface;
} __attribute__ ((packed)) usb_standard_interface_descriptor_t;

/** Standard USB endpoint descriptor.
 */
typedef struct {
	/** Size of this descriptor in bytes. */
	uint8_t length;
	/** Descriptor type (USB_DESCTYPE_ENDPOINT). */
	uint8_t descriptor_type;
	/** Endpoint address together with data flow direction. */
	uint8_t endpoint_address;
	/** Endpoint attributes.
	 * Includes transfer type (usb_transfer_type_t).
	 */
	uint8_t attributes;
	/** Maximum packet size. */
	uint16_t max_packet_size;
	/** Polling interval in milliseconds.
	 * Ignored for bulk and control endpoints.
	 * Isochronous endpoints must use value 1.
	 * Interrupt endpoints any value from 1 to 255.
	 */
	uint8_t poll_interval;
} __attribute__ ((packed)) usb_standard_endpoint_descriptor_t;

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
	usb_standard_hid_class_descriptor_info_t report_desc_info;
} __attribute__ ((packed)) usb_standard_hid_descriptor_t;

#endif
/**
 * @}
 */
