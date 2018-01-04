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
 * @brief USB interface definition.
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

extern usb_dev_session_t *usb_dev_connect(devman_handle_t);
extern usb_dev_session_t *usb_dev_connect_to_self(ddf_dev_t *);
extern void usb_dev_disconnect(usb_dev_session_t *);

extern errno_t usb_get_my_interface(async_exch_t *, int *);
extern errno_t usb_get_my_device_handle(async_exch_t *, devman_handle_t *);

extern errno_t usb_reserve_default_address(async_exch_t *, usb_speed_t);
extern errno_t usb_release_default_address(async_exch_t *);

extern errno_t usb_device_enumerate(async_exch_t *, unsigned port);
extern errno_t usb_device_remove(async_exch_t *, unsigned port);

extern errno_t usb_register_endpoint(async_exch_t *, usb_endpoint_t,
    usb_transfer_type_t, usb_direction_t, size_t, unsigned, unsigned);
extern errno_t usb_unregister_endpoint(async_exch_t *, usb_endpoint_t,
    usb_direction_t);
extern errno_t usb_read(async_exch_t *, usb_endpoint_t, uint64_t, void *, size_t,
    size_t *);
extern errno_t usb_write(async_exch_t *, usb_endpoint_t, uint64_t, const void *,
    size_t);

/** Callback for outgoing transfer. */
typedef void (*usb_iface_transfer_out_callback_t)(errno_t, void *);

/** Callback for incoming transfer. */
typedef void (*usb_iface_transfer_in_callback_t)(errno_t, size_t, void *);

/** USB device communication interface. */
typedef struct {
	errno_t (*get_my_interface)(ddf_fun_t *, int *);
	errno_t (*get_my_device_handle)(ddf_fun_t *, devman_handle_t *);

	errno_t (*reserve_default_address)(ddf_fun_t *, usb_speed_t);
	errno_t (*release_default_address)(ddf_fun_t *);

	errno_t (*device_enumerate)(ddf_fun_t *, unsigned);
	errno_t (*device_remove)(ddf_fun_t *, unsigned);

	errno_t (*register_endpoint)(ddf_fun_t *, usb_endpoint_t,
	    usb_transfer_type_t, usb_direction_t, size_t, unsigned, unsigned);
	errno_t (*unregister_endpoint)(ddf_fun_t *, usb_endpoint_t,
	    usb_direction_t);

	errno_t (*read)(ddf_fun_t *, usb_endpoint_t, uint64_t, uint8_t *, size_t,
	    usb_iface_transfer_in_callback_t, void *);
	errno_t (*write)(ddf_fun_t *, usb_endpoint_t, uint64_t, const uint8_t *,
	    size_t, usb_iface_transfer_out_callback_t, void *);
} usb_iface_t;

#endif
/**
 * @}
 */
