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
 * @brief Virtual host controller driver.
 */

#include <devmap.h>
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
#include "hc.h"
#include "devices.h"
#include "hub.h"
#include "conn.h"

static ddf_dev_ops_t vhc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &vhc_iface,
	.interfaces[USB_DEV_IFACE] = &vhc_usb_iface,
	.close = on_client_close,
	.default_handler = default_connection_handler
};

static int vhc_add_device(ddf_dev_t *dev)
{
	static int vhc_count = 0;
	int rc;

	/*
	 * Currently, we know how to simulate only single HC.
	 */
	if (vhc_count > 0) {
		return ELIMIT;
	}

	/*
	 * Create exposed function representing the host controller
	 * itself.
	 */
	ddf_fun_t *hc = ddf_fun_create(dev, fun_exposed, "hc");
	if (hc == NULL) {
		usb_log_fatal("Failed to create device function.\n");
		return ENOMEM;
	}

	hc->ops = &vhc_ops;

	rc = ddf_fun_bind(hc);
	if (rc != EOK) {
		usb_log_fatal("Failed to bind HC function: %s.\n",
		    str_error(rc));
		return rc;
	}

	ddf_fun_add_to_class(hc, "usbhc");

	/*
	 * Initialize our hub and announce its presence.
	 */
	virtual_hub_device_init(hc);

	usb_log_info("Virtual USB host controller ready (dev %zu, hc %zu).\n",
	    (size_t) dev->handle, (size_t) hc->handle);

	return EOK;
}

static driver_ops_t vhc_driver_ops = {
	.add_device = vhc_add_device,
};

static driver_t vhc_driver = {
	.name = NAME,
	.driver_ops = &vhc_driver_ops
};


int main(int argc, char * argv[])
{	
	/*
	 * Temporary workaround. Wait a little bit to be the last driver
	 * in devman output.
	 */
	//sleep(5);

	usb_log_enable(USB_LOG_LEVEL_DEFAULT, NAME);

	printf(NAME ": virtual USB host controller driver.\n");

	/*
	 * Initialize address management.
	 */
	address_init();

	/*
	 * Run the transfer scheduler.
	 */
	hc_manager();

	/*
	 * We are also a driver within devman framework.
	 */
	return ddf_driver_main(&vhc_driver);
}


/**
 * @}
 */
