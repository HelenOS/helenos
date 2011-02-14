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
 * @brief HC driver and hub driver.
 */
#ifndef LIBUSB_HCDHUBD_H_
#define LIBUSB_HCDHUBD_H_

#include <adt/list.h>
#include <bool.h>
#include <driver.h>
#include <usb/usb.h>

/** Endpoint properties. */
typedef struct {
	/** Endpoint number. */
	usb_endpoint_t endpoint;
	/** Transfer type. */
	usb_transfer_type_t transfer_type;
	/** Endpoint direction. */
	usb_direction_t direction;
	/** Data toggle bit. */
	int data_toggle;
} usb_hc_endpoint_info_t;

/** Information about attached USB device. */
typedef struct {
	/** Assigned address. */
	usb_address_t address;
	/** Number of endpoints. */
	size_t endpoint_count;
	/** Endpoint properties. */
	usb_hc_endpoint_info_t *endpoints;
	/** Link to other attached devices of the same HC. */
	link_t link;
} usb_hcd_attached_device_info_t;

/**
 * @}
 */

#endif
