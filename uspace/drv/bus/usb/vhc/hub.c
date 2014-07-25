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
#include <usbvirt/device.h>
#include <errno.h>
#include <async.h>
#include <str_error.h>
#include <stdlib.h>
#include <ddf/driver.h>
#include <devman.h>
#include <usb/dev/hub.h>
#include <usb/dev/recognise.h>

#include "hub.h"
#include "vhcd.h"
#include "conn.h"

usbvirt_device_t virtual_hub_device = {
	.name = "root hub",
	.ops = &hub_ops,
	.address = 0
};

static ddf_dev_ops_t rh_ops = {
	.interfaces[USB_DEV_IFACE] = &rh_usb_iface,
};

static int hub_register_in_devman_fibril(void *arg);

void virtual_hub_device_init(ddf_fun_t *hc_dev)
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

static int pretend_port_rest(void *unused2)
{
	return EOK;
}

/** Register root hub in devman.
 *
 * @param arg Host controller device (type <code>device_t *</code>).
 * @return Error code.
 */
int hub_register_in_devman_fibril(void *arg)
{
	ddf_fun_t *hc_dev = (ddf_fun_t *) arg;
	int rc;

	usb_hc_connection_t hc_conn;
	usb_hc_connection_initialize(&hc_conn, ddf_fun_get_handle(hc_dev));

	rc = usb_hc_connection_open(&hc_conn);
	assert(rc == EOK);

	ddf_fun_t *hub_dev;

	hub_dev = ddf_fun_create(ddf_fun_get_dev(hc_dev), fun_inner, NULL);
	if (hub_dev == NULL) {
		rc = ENOMEM;
		usb_log_fatal("Failed to create root hub: %s.\n",
		    str_error(rc));
		return rc;
	}

	rc = usb_hc_new_device_wrapper(ddf_fun_get_dev(hc_dev), hub_dev,
	    &hc_conn, USB_SPEED_FULL, pretend_port_rest, NULL, NULL, &rh_ops);
	if (rc != EOK) {
		usb_log_fatal("Failed to create root hub: %s.\n",
		    str_error(rc));
		ddf_fun_destroy(hub_dev);
	}

	usb_hc_connection_close(&hc_conn);

	usb_log_info("Created root hub function (handle %zu).\n",
	    (size_t) ddf_fun_get_handle(hub_dev));

	return 0;
}

/**
 * @}
 */
