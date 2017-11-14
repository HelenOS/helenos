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

extern int usbhc_reserve_default_address(async_exch_t *, usb_speed_t);
extern int usbhc_release_default_address(async_exch_t *);

extern int usbhc_device_enumerate(async_exch_t *, unsigned port);
extern int usbhc_device_remove(async_exch_t *, unsigned port);

extern int usbhc_register_endpoint(async_exch_t *, usb_endpoint_desc_t *);
extern int usbhc_unregister_endpoint(async_exch_t *, usb_endpoint_desc_t *);
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

	int (*register_endpoint)(ddf_fun_t *, usb_endpoint_desc_t *);
	int (*unregister_endpoint)(ddf_fun_t *, usb_endpoint_desc_t *);

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
