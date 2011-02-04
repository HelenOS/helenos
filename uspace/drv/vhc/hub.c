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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * @brief Virtual USB hub.
 */
#include <usb/classes/classes.h>
#include <usbvirt/hub.h>
#include <usbvirt/device.h>
#include <errno.h>
#include <str_error.h>
#include <stdlib.h>
#include <driver.h>
#include <usb/usbdrv.h>

#include "hub.h"
#include "hub/virthub.h"
#include "vhcd.h"

usbvirt_device_t virtual_hub_device;

static int hub_register_in_devman_fibril(void *arg);

void virtual_hub_device_init(device_t *hc_dev)
{
	virthub_init(&virtual_hub_device);

	/*
	 * We need to register the root hub.
	 * This must be done in separate fibril because the device
	 * we are connecting to are ourselves and we cannot connect
	 * before leaving the add_device() function.
	 */
	fid_t root_hub_registration
	    = fibril_create(hub_register_in_devman_fibril, hc_dev);
	if (root_hub_registration == 0) {
		usb_log_fatal("Failed to create hub registration fibril.\n");
		return;
	}

	fibril_add_ready(root_hub_registration);
}

/** Register root hub in devman.
 *
 * @param arg Host controller device (type <code>device_t *</code>).
 * @return Error code.
 */
int hub_register_in_devman_fibril(void *arg)
{
	device_t *hc_dev = (device_t *) arg;

	int hc;
	do {
		hc = usb_drv_hc_connect(hc_dev, hc_dev->handle,
		    IPC_FLAG_BLOCKING);
	} while (hc < 0);

	usb_drv_reserve_default_address(hc);

	usb_address_t hub_address = usb_drv_request_address(hc);
	usb_drv_req_set_address(hc, USB_ADDRESS_DEFAULT, hub_address);

	usb_drv_release_default_address(hc);

	devman_handle_t hub_handle;
	usb_drv_register_child_in_devman(hc, hc_dev, hub_address, &hub_handle);
	usb_drv_bind_address(hc, hub_address, hub_handle);

	return EOK;
}
	

/**
 * @}
 */
