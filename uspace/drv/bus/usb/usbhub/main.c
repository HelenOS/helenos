/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhub
 * @{
 */

#include <ddf/driver.h>
#include <errno.h>
#include <async.h>
#include <stdio.h>
#include <io/logctl.h>

#include <usb/dev/driver.h>
#include <usb/classes/classes.h>
#include <usb/debug.h>

#include "usbhub.h"

/** USB hub driver operations. */
static const usb_driver_ops_t usb_hub_driver_ops = {
	.device_add = usb_hub_device_add,
	.device_remove = usb_hub_device_remove,
	.device_gone = usb_hub_device_gone,
};

/** Static usb hub driver information. */
static const usb_driver_t usb_hub_driver = {
	.name = NAME,
	.ops = &usb_hub_driver_ops,
	.endpoints = usb_hub_endpoints
};

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS USB hub driver.\n");
	log_init(NAME);
	logctl_set_log_level(NAME, LVL_NOTE);
	return usb_driver_main(&usb_hub_driver);
}

/**
 * @}
 */
