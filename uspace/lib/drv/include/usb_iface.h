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

/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */
/** @file
 * @brief USB device interface definition.
 */

#ifndef LIBDRV_USB_IFACE_H_
#define LIBDRV_USB_IFACE_H_

#include "ddf/driver.h"
#include <async.h>

typedef async_sess_t usb_dev_session_t;

/** USB speeds. */
typedef enum {
	/** USB 1.1 low speed (1.5Mbits/s). */
	USB_SPEED_LOW,
	/** USB 1.1 full speed (12Mbits/s). */
	USB_SPEED_FULL,
	/** USB 2.0 high speed (480Mbits/s). */
	USB_SPEED_HIGH,
	/** USB 3.0 super speed (5Gbits/s). */
	USB_SPEED_SUPER,
	/** Psuedo-speed serving as a boundary. */
	USB_SPEED_MAX
} usb_speed_t;

/** USB endpoint number type.
 * Negative values could be used to indicate error.
 */
typedef int16_t usb_endpoint_t;

/** USB address type.
 * Negative values could be used to indicate error.
 */
typedef int16_t usb_address_t;

/** USB address for the purposes of Transaction Translation.
 */
typedef struct {
	usb_address_t address;
	unsigned port;
} usb_tt_address_t;

/** USB transfer type. */
typedef enum {
	USB_TRANSFER_CONTROL = 0,
	USB_TRANSFER_ISOCHRONOUS = 1,
	USB_TRANSFER_BULK = 2,
	USB_TRANSFER_INTERRUPT = 3
} usb_transfer_type_t;

/** USB data transfer direction. */
typedef enum {
	USB_DIRECTION_IN,
	USB_DIRECTION_OUT,
	USB_DIRECTION_BOTH
} usb_direction_t;

/** USB complete address type.
 * Pair address + endpoint is identification of transaction recipient.
 */
typedef union {
	struct {
		usb_address_t address;
		usb_endpoint_t endpoint;
	} __attribute__((packed));
	uint32_t packed;
} usb_target_t;

/** Description of usb endpoint.
 */
typedef struct {
	unsigned max_burst;
	unsigned max_streams;
	unsigned mult;
	unsigned bytes_per_interval;
} usb3_endpoint_desc_t;

typedef struct {
	unsigned polling_interval;
} usb2_endpoint_desc_t;

typedef struct usb_endpoint_desc {
	/** Endpoint number. */
	usb_endpoint_t endpoint_no;

	/** Endpoint transfer type. */
	usb_transfer_type_t transfer_type;

	/** Endpoint direction. */
	usb_direction_t direction;

	/** Maximum packet size for the endpoint. */
	size_t max_packet_size;

	/** Scheduling interval for HC. Only valid for interrupt/isoch transfer. */
	size_t interval;

	/** Number of packets per frame/uframe.
	 * Only valid for HS INT and ISO transfers. All others should set to 1*/
	unsigned packets;

	/** Bus version specific information */
	usb2_endpoint_desc_t usb2;
	usb3_endpoint_desc_t usb3;
} usb_endpoint_desc_t;


extern usb_dev_session_t *usb_dev_connect(devman_handle_t);
extern usb_dev_session_t *usb_dev_connect_to_self(ddf_dev_t *);
extern void usb_dev_disconnect(usb_dev_session_t *);

extern int usb_get_my_interface(async_exch_t *, int *);
extern int usb_get_my_device_handle(async_exch_t *, devman_handle_t *);

/** USB device communication interface. */
typedef struct {
	int (*get_my_interface)(ddf_fun_t *, int *);
	int (*get_my_device_handle)(ddf_fun_t *, devman_handle_t *);
} usb_iface_t;

#endif
/**
 * @}
 */
