/*
 * Copyright (c) 2010 Vojtech Horky
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
 * @brief Hub driver.
 */

#ifndef DRV_USBHUB_USBHUB_H
#define DRV_USBHUB_USBHUB_H

#include <ddf/driver.h>

#include <usb/classes/hub.h>

#include <usb/dev/pipes.h>
#include <usb/dev/driver.h>

#include <fibril_synch.h>

#define NAME "usbhub"

#include "port.h"

/** Information about attached hub. */
struct usb_hub_dev {
	/** Number of ports. */
	size_t port_count;
	/** Port structures, one for each port */
	usb_hub_port_t *ports;
	/** Generic usb device data*/
	usb_device_t *usb_device;

	/** Number of pending operations on the mutex to prevent shooting
	 * ourselves in the foot.
	 * When the hub is disconnected but we are in the middle of some
	 * operation, we cannot destroy this structure right away because
	 * the pending operation might use it.
	 */
	size_t pending_ops_count;
	/** Guard for pending_ops_count. */
	fibril_mutex_t pending_ops_mutex;
	/** Condition variable for pending_ops_count. */
	fibril_condvar_t pending_ops_cv;
	/** Pointer to usbhub function. */
	ddf_fun_t *hub_fun;
	/** Status indicator */
	bool running;
	/** Hub supports port power switching. */
	bool power_switched;
	/** Each port is switched individually. */
	bool per_port_power;
};

extern const usb_endpoint_description_t hub_status_change_endpoint_description;

extern errno_t usb_hub_device_add(usb_device_t *);
extern errno_t usb_hub_device_remove(usb_device_t *);
extern errno_t usb_hub_device_gone(usb_device_t *);

extern bool hub_port_changes_callback(usb_device_t *, uint8_t *, size_t,
    void *);

#endif

/**
 * @}
 */
