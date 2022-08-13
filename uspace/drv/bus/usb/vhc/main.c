/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * Virtual host controller.
 */

#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>

#include <usb/host/ddf_helpers.h>
#include <usb/host/utility.h>

#include <usb/debug.h>
#include "vhcd.h"

static ddf_dev_ops_t vhc_ops = {
	.close = on_client_close,
	.default_handler = default_connection_handler
};

static errno_t vhc_control_node(ddf_dev_t *dev, ddf_fun_t **fun)
{
	assert(dev);
	assert(fun);

	*fun = ddf_fun_create(dev, fun_exposed, "virtual");
	if (!*fun)
		return ENOMEM;

	vhc_data_t *vhc = ddf_fun_data_alloc(*fun, sizeof(vhc_data_t));
	if (!vhc) {
		ddf_fun_destroy(*fun);
	}
	ddf_fun_set_ops(*fun, &vhc_ops);
	const errno_t ret = ddf_fun_bind(*fun);
	if (ret != EOK) {
		ddf_fun_destroy(*fun);
		*fun = NULL;
		return ret;
	}
	return EOK;
}

static errno_t vhc_dev_add(ddf_dev_t *dev)
{
	/* Initialize generic structures */
	errno_t ret = hcd_ddf_setup_hc(dev, sizeof(vhc_data_t));
	if (ret != EOK) {
		usb_log_error("Failed to init HCD structures: %s.",
		    str_error(ret));
		return ret;
	}
	vhc_data_t *vhc = ddf_dev_data_get(dev);
	vhc_init(vhc);

	hc_device_setup(&vhc->base, (bus_t *) &vhc->bus);

	/* Initialize virtual structure */
	ddf_fun_t *ctl_fun = NULL;
	ret = vhc_control_node(dev, &ctl_fun);
	if (ret != EOK) {
		usb_log_error("Failed to setup control node.");
		return ret;
	}

	/* Add virtual hub device */
	ret = vhc_virtdev_plug_hub(vhc, &vhc->hub, NULL, 0);
	if (ret != EOK) {
		usb_log_error("Failed to plug root hub: %s.", str_error(ret));
		ddf_fun_destroy(ctl_fun);
		return ret;
	}

	/*
	 * Creating root hub registers a new USB device so HC
	 * needs to be ready at this time.
	 */
	ret = hc_setup_virtual_root_hub(&vhc->base, USB_SPEED_HIGH);
	if (ret != EOK) {
		usb_log_error("Failed to init VHC root hub: %s",
		    str_error(ret));
		// TODO do something here...
	}

	return ret;
}

static driver_ops_t vhc_driver_ops = {
	.dev_add = vhc_dev_add,
};

static driver_t vhc_driver = {
	.name = NAME,
	.driver_ops = &vhc_driver_ops
};

int main(int argc, char *argv[])
{
	log_init(NAME);
	printf(NAME ": virtual USB host controller driver.\n");

	return ddf_driver_main(&vhc_driver);
}

/**
 * @}
 */
