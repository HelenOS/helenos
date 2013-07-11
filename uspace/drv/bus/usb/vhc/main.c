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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * Virtual host controller.
 */

#include <loc.h>
#include <async.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysinfo.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>

#include <usb/usb.h>
#include <usb/ddfiface.h>
#include <usb_iface.h>
#include "vhcd.h"
#include "hub.h"
#include "conn.h"

static ddf_dev_ops_t vhc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &vhc_iface,
	.interfaces[USB_DEV_IFACE] = &vhc_usb_iface,
	.close = on_client_close,
	.default_handler = default_connection_handler
};

static int vhc_dev_add(ddf_dev_t *dev)
{
	static int vhc_count = 0;
	int rc;

	if (vhc_count > 0) {
		return ELIMIT;
	}

	vhc_data_t *data = ddf_dev_data_alloc(dev, sizeof(vhc_data_t));
	if (data == NULL) {
		usb_log_fatal("Failed to allocate memory.\n");
		return ENOMEM;
	}
	data->magic = 0xDEADBEEF;
	rc = usb_endpoint_manager_init(&data->ep_manager, (size_t) -1,
	    bandwidth_count_usb11);
	if (rc != EOK) {
		usb_log_fatal("Failed to initialize endpoint manager.\n");
		free(data);
		return rc;
	}
	usb_device_manager_init(&data->dev_manager, USB_SPEED_MAX);

	ddf_fun_t *hc = ddf_fun_create(dev, fun_exposed, "hc");
	if (hc == NULL) {
		usb_log_fatal("Failed to create device function.\n");
		free(data);
		return ENOMEM;
	}

	ddf_fun_set_ops(hc, &vhc_ops);
	list_initialize(&data->devices);
	fibril_mutex_initialize(&data->guard);
	data->hub = &virtual_hub_device;
	data->hc_fun = hc;

	rc = ddf_fun_bind(hc);
	if (rc != EOK) {
		usb_log_fatal("Failed to bind HC function: %s.\n",
		    str_error(rc));
		free(data);
		return rc;
	}

	rc = ddf_fun_add_to_category(hc, USB_HC_CATEGORY);
	if (rc != EOK) {
		usb_log_fatal("Failed to add function to HC class: %s.\n",
		    str_error(rc));
		free(data);
		return rc;
	}

	virtual_hub_device_init(hc);

	usb_log_info("Virtual USB host controller ready (dev %zu, hc %zu).\n",
	    (size_t) ddf_dev_get_handle(dev), (size_t) ddf_fun_get_handle(hc));

	rc = vhc_virtdev_plug_hub(data, data->hub, NULL);
	if (rc != EOK) {
		usb_log_fatal("Failed to plug root hub: %s.\n", str_error(rc));
		free(data);
		return rc;
	}

	return EOK;
}

static driver_ops_t vhc_driver_ops = {
	.dev_add = vhc_dev_add,
};

static driver_t vhc_driver = {
	.name = NAME,
	.driver_ops = &vhc_driver_ops
};


int main(int argc, char * argv[])
{	
	log_init(NAME);

	printf(NAME ": virtual USB host controller driver.\n");

	return ddf_driver_main(&vhc_driver);
}


/**
 * @}
 */
