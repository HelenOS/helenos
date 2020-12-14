/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek
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
#include <fibril_synch.h>

#include <usb/classes/hub.h>

#include <usb/dev/pipes.h>
#include <usb/dev/driver.h>
#include <usb/dev/poll.h>

#define NAME "usbhub"

#include "port.h"
#include "status.h"

/** Information about attached hub. */
struct usb_hub_dev {
	/** Number of ports. */
	size_t port_count;
	/** Port structures, one for each port */
	usb_hub_port_t *ports;
	/** Speed of the hub */
	usb_speed_t speed;
	/** Generic usb device data */
	usb_device_t *usb_device;
	/** Data polling handle. */
	usb_polling_t polling;
	/** Pointer to usbhub function. */
	ddf_fun_t *hub_fun;
	/** Device communication pipe. */
	usb_pipe_t *control_pipe;
	/** Hub supports port power switching. */
	bool power_switched;
	/** Each port is switched individually. */
	bool per_port_power;
	/** Whether MTT is available */
	bool mtt_available;
};

extern const usb_endpoint_description_t *usb_hub_endpoints [];

errno_t usb_hub_device_add(usb_device_t *);
errno_t usb_hub_device_remove(usb_device_t *);
errno_t usb_hub_device_gone(usb_device_t *);

errno_t usb_hub_set_depth(const usb_hub_dev_t *);
errno_t usb_hub_get_port_status(const usb_hub_dev_t *, size_t,
    usb_port_status_t *);
errno_t usb_hub_set_port_feature(const usb_hub_dev_t *, size_t,
    usb_hub_class_feature_t);
errno_t usb_hub_clear_port_feature(const usb_hub_dev_t *, size_t,
    usb_hub_class_feature_t);

bool hub_port_changes_callback(usb_device_t *, uint8_t *, size_t, void *);

errno_t usb_hub_reserve_default_address(usb_hub_dev_t *, async_exch_t *,
    usb_port_t *);
errno_t usb_hub_release_default_address(usb_hub_dev_t *, async_exch_t *);

#endif

/**
 * @}
 */
