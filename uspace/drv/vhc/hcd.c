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

/** @addtogroup usb
 * @{
 */ 
/** @file
 * @brief Virtual host controller driver.
 */

#include <devmap.h>
#include <ipc/ipc.h>
#include <async.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysinfo.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <driver.h>

#include <usb/usb.h>
#include "vhcd.h"
#include "hc.h"
#include "devices.h"
#include "hub.h"
#include "conn.h"

static device_ops_t vhc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &vhc_iface,
	.default_handler = default_connection_handler
};

static int vhc_count = 0;
static int vhc_add_device(device_t *dev)
{
	/*
	 * Currently, we know how to simulate only single HC.
	 */
	if (vhc_count > 0) {
		return ELIMIT;
	}

	vhc_count++;

	dev->ops = &vhc_ops;

	/*
	 * Initialize address management.
	 */
	address_init();

	/*
	 * Initialize our hub and announce its presence.
	 */
	hub_init();
	usb_hcd_add_root_hub(dev);

	printf("%s: virtual USB host controller ready.\n", NAME);

	return EOK;
}

static driver_ops_t vhc_driver_ops = {
	.add_device = vhc_add_device,
};

static driver_t vhc_driver = {
	.name = NAME,
	.driver_ops = &vhc_driver_ops
};

/** Fibril wrapper for HC transaction manager.
 *
 * @param arg Not used.
 * @return Nothing, return argument is unreachable.
 */
static int hc_manager_fibril(void *arg)
{
	hc_manager();
	return EOK;
}

int main(int argc, char * argv[])
{	
	printf("%s: virtual USB host controller driver.\n", NAME);

	debug_level = 10;

	fid_t fid = fibril_create(hc_manager_fibril, NULL);
	if (fid == 0) {
		printf("%s: failed to start HC manager fibril\n", NAME);
		return ENOMEM;
	}
	fibril_add_ready(fid);

	/*
	 * Temporary workaround. Wait a little bit to be the last driver
	 * in devman output.
	 */
	sleep(4);

	return driver_main(&vhc_driver);
}


/**
 * @}
 */
