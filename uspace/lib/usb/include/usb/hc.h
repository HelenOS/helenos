/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
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
 * General communication with host controller.
 */
#ifndef LIBUSB_HC_H_
#define LIBUSB_HC_H_

#include <async.h>
#include <devman.h>
#include <ddf/driver.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <usb/usb.h>

/** Connection to the host controller driver.
 *
 * This is a high level IPC communication wrapper. After the structure has been
 * initialized using devman handle of an USB host controller, it
 * will manage all communication to that host controller, including session
 * creation/destruction and proper IPC protocol.
 */
typedef struct {
	/** Devman handle of the host controller. */
	devman_handle_t hc_handle;
	/** Session to the host controller. */
	async_sess_t *hc_sess;
	/** Session guard. */
	fibril_mutex_t guard;
	/** Use counter. */
	unsigned ref_count;
} usb_hc_connection_t;

/** Initialize connection to USB host controller.
 *
 * @param connection Connection to be initialized.
 * @param hc_handle Devman handle of the host controller.
 * @return Error code.
 */
static inline void usb_hc_connection_initialize(usb_hc_connection_t *connection,
    devman_handle_t hc_handle)
{
	assert(connection);
	connection->hc_handle = hc_handle;
	connection->hc_sess = NULL;
	connection->ref_count = 0;
	fibril_mutex_initialize(&connection->guard);
}

int usb_hc_connection_initialize_from_device(usb_hc_connection_t *, ddf_dev_t *);

void usb_hc_connection_deinitialize(usb_hc_connection_t *);

int usb_hc_connection_open(usb_hc_connection_t *);
int usb_hc_connection_close(usb_hc_connection_t *);

usb_address_t usb_hc_request_address(usb_hc_connection_t *, usb_address_t, bool,
    usb_speed_t);
int usb_hc_bind_address(usb_hc_connection_t *, usb_address_t, devman_handle_t);
int usb_hc_get_handle_by_address(usb_hc_connection_t *, usb_address_t,
    devman_handle_t *);
int usb_hc_release_address(usb_hc_connection_t *, usb_address_t);

int usb_hc_register_endpoint(usb_hc_connection_t *, usb_address_t,
    usb_endpoint_t, usb_transfer_type_t, usb_direction_t, size_t, unsigned int);
int usb_hc_unregister_endpoint(usb_hc_connection_t *, usb_address_t,
    usb_endpoint_t, usb_direction_t);

int usb_hc_read(usb_hc_connection_t *, usb_address_t, usb_endpoint_t,
    uint64_t, void *, size_t, size_t *);
int usb_hc_write(usb_hc_connection_t *, usb_address_t, usb_endpoint_t,
    uint64_t, const void *, size_t);

/** Get host controller handle by its class index.
 *
 * @param sid Service ID of the HC function.
 * @param hc_handle Where to store the HC handle
 *	(can be NULL for existence test only).
 * @return Error code.
 */
static inline int usb_ddf_get_hc_handle_by_sid(
    service_id_t sid, devman_handle_t *handle)
{
	devman_handle_t h;
	return devman_fun_sid_to_handle(sid, handle ? handle : &h);
}

#endif
/**
 * @}
 */
