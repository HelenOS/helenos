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

/** @addtogroup libusbvirt usb
 * @{
 */
/** @file
 * @brief Virtual USB device.
 */
#ifndef LIBUSBVIRT_DEVICE_H_
#define LIBUSBVIRT_DEVICE_H_

#include <usb/hcd.h>
#include <usb/descriptor.h>
#include <usb/devreq.h>

struct usbvirt_device;

typedef int (*usbvirt_on_device_request_t)(struct usbvirt_device *dev,
	usb_device_request_setup_packet_t *request,
	uint8_t *data);

typedef struct {
	usbvirt_on_device_request_t on_devreq_std;
	usbvirt_on_device_request_t on_devreq_class;
	int (*on_data)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
} usbvirt_device_ops_t;

typedef struct usbvirt_device {
	/** Callback device operations. */
	usbvirt_device_ops_t *ops;
	
	/** Send data to HC.
	 * @warning Do not change after initializing with
	 * usbvirt_device_init().
	 * This function is here merely to make the interface more OOP.
	 */
	int (*send_data)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	
	
	
	/* Device attributes. */
	
	/** Standard device descriptor.
	 * If this descriptor is set (i.e. not NULL), the framework
	 * automatically handles call for its retrieval.
	 */
	usb_standard_device_descriptor_t *standard_descriptor;
	
	
	/* Private attributes. */
	
	/** Phone to HC.
	 * @warning Do not change, this is private variable.
	 */
	int vhcd_phone_;
	
	/** Device id.
	 * This item will be removed when device enumeration and
	 * recognition is implemented.
	 */
	int device_id_;
} usbvirt_device_t;

#endif
/**
 * @}
 */
