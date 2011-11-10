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
/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * Hub ports related functions.
 */
#ifndef DRV_USBHUB_PORT_H
#define DRV_USBHUB_PORT_H

#include <usb/dev/driver.h>
#include <usb/dev/hub.h>
#include <usb/classes/hub.h>

typedef struct usb_hub_dev usb_hub_dev_t;

/** Information about single port on a hub. */
typedef struct {
	/* Port number as reported in descriptors. */
	size_t port_number;
	/** Device communication pipe. */
	usb_pipe_t *control_pipe;
	/** Mutex needed not only by CV for checking port reset. */
	fibril_mutex_t mutex;
	/** CV for waiting to port reset completion. */
	fibril_condvar_t reset_cv;
	/** Whether port reset is completed.
	 * Guarded by @c reset_mutex.
	 */
	bool reset_completed;
	/** Whether to announce the port reset as successful. */
	bool reset_okay;

	/** Information about attached device. */
	usb_hub_attached_device_t attached_device;
} usb_hub_port_t;

/** Initialize hub port information.
 *
 * @param port Port to be initialized.
 */
static inline void usb_hub_port_init(usb_hub_port_t *port, size_t port_number,
    usb_pipe_t *control_pipe)
{
	assert(port);
	port->attached_device.address = -1;
	port->attached_device.fun = NULL;
	port->port_number = port_number;
	port->control_pipe = control_pipe;
	fibril_mutex_initialize(&port->mutex);
	fibril_condvar_initialize(&port->reset_cv);
}
int usb_hub_port_fini(usb_hub_port_t *port, usb_hub_dev_t *hub);
int usb_hub_port_clear_feature(
    usb_hub_port_t *port, usb_hub_class_feature_t feature);
int usb_hub_port_set_feature(
    usb_hub_port_t *port, usb_hub_class_feature_t feature);
void usb_hub_port_reset_fail(usb_hub_port_t *port);
void usb_hub_port_process_interrupt(usb_hub_port_t *port, usb_hub_dev_t *hub);

#endif
/**
 * @}
 */
