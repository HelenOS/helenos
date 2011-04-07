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

/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * Hub ports related functions.
 */
#ifndef DRV_USBHUB_PORTS_H
#define DRV_USBHUB_PORTS_H

#include <ipc/devman.h>
#include <usb/usb.h>
#include <ddf/driver.h>
#include <fibril_synch.h>

#include <usb/hub.h>

#include <usb/pipes.h>
#include <usb/devdrv.h>

/** Information about single port on a hub. */
typedef struct {
	/** Mutex needed by CV for checking port reset. */
	fibril_mutex_t reset_mutex;
	/** CV for waiting to port reset completion. */
	fibril_condvar_t reset_cv;
	/** Whether port reset is completed.
	 * Guarded by @c reset_mutex.
	 */
	bool reset_completed;

	/** Information about attached device. */
	usb_hc_attached_device_t attached_device;
} usb_hub_port_t;

/** Initialize hub port information.
 *
 * @param port Port to be initialized.
 */
static inline void usb_hub_port_init(usb_hub_port_t *port) {
	port->attached_device.address = -1;
	port->attached_device.handle = 0;
	fibril_mutex_initialize(&port->reset_mutex);
	fibril_condvar_initialize(&port->reset_cv);
}

bool hub_port_changes_callback(usb_device_t *, uint8_t *, size_t, void *);


#endif
/**
 * @}
 */
