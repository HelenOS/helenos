/*
 * Copyright (c) 2011 Vojtech Horky
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
 * General communication with host controller driver.
 */
#ifndef LIBUSB_HC_H_
#define LIBUSB_HC_H_

#include <sys/types.h>
#include <ipc/devman.h>
#include <ipc/loc.h>
#include <ddf/driver.h>
#include <bool.h>
#include <async.h>
#include <usb/usb.h>

/** Connection to the host controller driver. */
typedef struct {
	/** Devman handle of the host controller. */
	devman_handle_t hc_handle;
	/** Session to the host controller. */
	async_sess_t *hc_sess;
} usb_hc_connection_t;

int usb_hc_connection_initialize_from_device(usb_hc_connection_t *,
    const ddf_dev_t *);
int usb_hc_connection_initialize(usb_hc_connection_t *, devman_handle_t);

int usb_hc_connection_open(usb_hc_connection_t *);
bool usb_hc_connection_is_opened(const usb_hc_connection_t *);
int usb_hc_connection_close(usb_hc_connection_t *);
int usb_hc_get_handle_by_address(usb_hc_connection_t *, usb_address_t,
    devman_handle_t *);

usb_address_t usb_hc_get_address_by_handle(devman_handle_t);

int usb_hc_find(devman_handle_t, devman_handle_t *);

int usb_resolve_device_handle(const char *, devman_handle_t *, usb_address_t *,
    devman_handle_t *);

int usb_ddf_get_hc_handle_by_sid(service_id_t, devman_handle_t *);


#endif
/**
 * @}
 */
