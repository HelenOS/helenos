/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2018 Ondrej Hlavaty, Michal Staruch
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
#include <usbhc_iface.h>

typedef async_sess_t usb_dev_session_t;

typedef struct {
	usb_address_t address;	/**< Current USB address */
	uint8_t depth;		/**< Depth in the hub hiearchy */
	usb_speed_t speed;	/**< Speed of the device */
	devman_handle_t handle;	/**< Handle to DDF function of the HC driver */
	/** Interface set by multi interface driver,  -1 if none */
	int iface;
} usb_device_desc_t;

extern usb_dev_session_t *usb_dev_connect(devman_handle_t);
extern usb_dev_session_t *usb_dev_connect_to_self(ddf_dev_t *);
extern void usb_dev_disconnect(usb_dev_session_t *);

extern errno_t usb_get_my_description(async_exch_t *, usb_device_desc_t *);

/** USB device communication interface. */
typedef struct {
	errno_t (*get_my_description)(ddf_fun_t *, usb_device_desc_t *);
} usb_iface_t;

#endif
/**
 * @}
 */
