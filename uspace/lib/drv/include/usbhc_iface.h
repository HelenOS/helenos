/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2017 Ondrej Hlavaty
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
 * @brief USB host controler interface definition. This is the interface of
 * USB host controller function, which can be used by usb device drivers.
 */

#ifndef LIBDRV_USBHC_IFACE_H_
#define LIBDRV_USBHC_IFACE_H_

#include "ddf/driver.h"
#include "usb_iface.h"
#include <async.h>

typedef struct usb_pipe_desc {
	/** Endpoint number. */
	usb_endpoint_t endpoint_no;

	/** Endpoint transfer type. */
	usb_transfer_type_t transfer_type;

	/** Endpoint direction. */
	usb_direction_t direction;

	/** Maximum size of one transfer */
	size_t max_transfer_size;
} usb_pipe_desc_t;

/** This structure follows standard endpoint descriptor + superspeed companion
 * descriptor, and exists to avoid dependency of libdrv on libusb. Keep the
 * internal fields named exactly like their source (because we want to use the
 * same macros to access them).
 * Callers shall fill it with bare contents of respective descriptors (in usb endianity).
 */
typedef struct {
	struct {
		uint8_t endpoint_address;
		uint8_t attributes;
		uint16_t max_packet_size;
		uint8_t poll_interval;
	} endpoint;

	/* Superspeed companion descriptor */
	struct companion_desc_t {
		uint8_t max_burst;
		uint8_t attributes;
		uint16_t bytes_per_interval;
	} companion;
} usb_endpoint_descriptors_t;

extern int usbhc_reserve_default_address(async_exch_t *, usb_speed_t);
extern int usbhc_release_default_address(async_exch_t *);

extern int usbhc_device_enumerate(async_exch_t *, unsigned port);
extern int usbhc_device_remove(async_exch_t *, unsigned port);

extern int usbhc_register_endpoint(async_exch_t *, usb_pipe_desc_t *, const usb_endpoint_descriptors_t *);
extern int usbhc_unregister_endpoint(async_exch_t *, const usb_pipe_desc_t *);

extern int usbhc_read(async_exch_t *, usb_endpoint_t, uint64_t, void *, size_t,
    size_t *);
extern int usbhc_write(async_exch_t *, usb_endpoint_t, uint64_t, const void *,
    size_t);

/** Callback for outgoing transfer */
typedef int (*usbhc_iface_transfer_callback_t)(void *, int, size_t);

/** USB device communication interface. */
typedef struct {
	int (*reserve_default_address)(ddf_fun_t *, usb_speed_t);
	int (*release_default_address)(ddf_fun_t *);

	int (*device_enumerate)(ddf_fun_t *, unsigned);
	int (*device_remove)(ddf_fun_t *, unsigned);

	int (*register_endpoint)(ddf_fun_t *, usb_pipe_desc_t *, const usb_endpoint_descriptors_t *);
	int (*unregister_endpoint)(ddf_fun_t *, const usb_pipe_desc_t *);

	int (*read)(ddf_fun_t *, usb_target_t,
		uint64_t, char *, size_t,
		usbhc_iface_transfer_callback_t, void *);
	int (*write)(ddf_fun_t *, usb_target_t,
		uint64_t, const char *, size_t,
		usbhc_iface_transfer_callback_t, void *);
} usbhc_iface_t;

#endif
/**
 * @}
 */
