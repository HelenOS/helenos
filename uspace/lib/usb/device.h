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
 * @brief USB device related types.
 */
#ifndef LIBUSB_DEVICE_H_
#define LIBUSB_DEVICE_H_

#include <ipc/ipc.h>
#include <async.h>

/** Standard USB device descriptor.
 */
typedef struct {
	/** Size of this descriptor in bytes. */
	uint8_t length;
	/** Device descriptor type. */
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
	uint8_t manufacturer;
	/** Product descriptor index. */
	uint8_t product;
	/** Device serial number desriptor index. */
	uint8_t serial_number;
	/** Number of possible configurations. */
	uint8_t configuration_count;
} __attribute__ ((packed)) usb_standard_device_descriptor_t;



#endif
/**
 * @}
 */
