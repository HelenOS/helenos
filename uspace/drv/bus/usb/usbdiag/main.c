/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup drvusbdiag
 * @{
 */
/**
 * @file
 * Main routines of USB diagnostic device driver.
 */
#include <errno.h>
#include <usb/debug.h>
#include <usb/dev/driver.h>
#include <usbdiag_iface.h>
#include <str_error.h>

#include "usbdiag.h"
#include "device.h"

#define NAME "usbdiag"

static int device_add(usb_device_t *dev)
{
	int rc;
	usb_log_info("Adding device '%s'", usb_device_get_name(dev));

	usb_diag_dev_t *diag_dev;
	if ((rc = usb_diag_dev_create(dev, &diag_dev))) {
		usb_log_error("Failed create device: %s.\n", str_error(rc));
		goto err;
	}

	if ((rc = ddf_fun_bind(diag_dev->fun))) {
		usb_log_error("Failed to bind DDF function: %s.\n", str_error(rc));
		goto err_create;
	}

	if ((rc = ddf_fun_add_to_category(diag_dev->fun, USBDIAG_CATEGORY))) {
		usb_log_error("Failed add DDF to category '"
		    USBDIAG_CATEGORY "': %s.\n", str_error(rc));
		goto err_bind;
	}

	return EOK;

err_bind:
	ddf_fun_unbind(diag_dev->fun);
err_create:
	usb_diag_dev_destroy(diag_dev);
err:
	return rc;
}

static int device_remove(usb_device_t *dev)
{
	int rc;
	usb_log_info("Removing device '%s'", usb_device_get_name(dev));

	usb_diag_dev_t *diag_dev = usb_device_to_usb_diag_dev(dev);

	/* TODO: Make sure nothing is going on with the device. */

	if ((rc = ddf_fun_unbind(diag_dev->fun))) {
		usb_log_error("Failed to unbind DDF function: %s\n", str_error(rc));
		goto err;
	}

	usb_diag_dev_destroy(diag_dev);

	return EOK;

err:
	return rc;
}

static int device_gone(usb_device_t *dev)
{
	usb_log_info("Device '%s' gone.", usb_device_get_name(dev));

	usb_diag_dev_t *diag_dev = usb_device_to_usb_diag_dev(dev);

	/* TODO: Make sure nothing is going on with the device. */
	/* TODO: Unregister device DDF function. */
	/* TODO: Remove device from list */

	usb_diag_dev_destroy(diag_dev);

	return EOK;
}

static int function_online(ddf_fun_t *fun)
{
	return ddf_fun_online(fun);
}

static int function_offline(ddf_fun_t *fun)
{
	return ddf_fun_offline(fun);
}

/** USB diagnostic driver ops. */
static const usb_driver_ops_t diag_driver_ops = {
	.device_add = device_add,
	.device_rem = device_remove,
	.device_gone = device_gone,
	.function_online = function_online,
	.function_offline = function_offline
};

/** USB diagnostic driver. */
static const usb_driver_t diag_driver = {
	.name = NAME,
	.ops = &diag_driver_ops,
	.endpoints = NULL
};

int main(int argc, char *argv[])
{
	printf(NAME ": USB diagnostic device driver.\n");

	log_init(NAME);

	return usb_driver_main(&diag_driver);
}

/**
 * @}
 */