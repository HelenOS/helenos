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

/** @addtogroup drvusbmid
 * @{
 */
/**
 * @file
 * Main routines of USB multi interface device driver.
 */
#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/dev/request.h>
#include <usb/descriptor.h>
#include <usb/dev/pipes.h>

#include "usbmid.h"

/** Callback when new MID device is attached to the host.
 *
 * @param gen_dev Generic DDF device representing the new device.
 * @return Error code.
 */
static errno_t usbmid_device_add(usb_device_t *dev)
{
	usb_log_info("Taking care of new MID `%s'.\n", usb_device_get_name(dev));

	return usbmid_explore_device(dev);
}

/** Callback when a MID device is about to be removed from the host.
 *
 * @param dev USB device representing the removed device.
 * @return Error code.
 */
static errno_t usbmid_device_remove(usb_device_t *dev)
{
	assert(dev);
	usb_mid_t *usb_mid = usb_device_data_get(dev);
	assert(usb_mid);

	/* Remove ctl function */
	errno_t ret = ddf_fun_unbind(usb_mid->ctl_fun);
	if (ret != EOK) {
		usb_log_error("Failed to unbind USB MID ctl function: %s.\n",
		    str_error(ret));
		return ret;
	}
	ddf_fun_destroy(usb_mid->ctl_fun);

	/* Remove all children */
	while (!list_empty(&usb_mid->interface_list)) {
		link_t *item = list_first(&usb_mid->interface_list);
		list_remove(item);

		usbmid_interface_t *iface = usbmid_interface_from_link(item);

		usb_log_info("Removing child `%s'.\n",
		    ddf_fun_get_name(iface->fun));

		/* Tell the child to go off-line. */
		errno_t pret = ddf_fun_offline(iface->fun);
		if (pret != EOK) {
			usb_log_warning("Failed to turn off child `%s': %s\n",
			    ddf_fun_get_name(iface->fun), str_error(pret));
			ret = pret;
		}

		/* Now remove the child. */
		pret = usbmid_interface_destroy(iface);
		if (pret != EOK) {
			usb_log_error("Failed to destroy child `%s': %s\n",
			    ddf_fun_get_name(iface->fun), str_error(pret));
			ret = pret;
		}
	}
	return ret;
}

/** Callback when a MID device was removed from the host.
 *
 * @param dev USB device representing the removed device.
 * @return Error code.
 */
static errno_t usbmid_device_gone(usb_device_t *dev)
{
	assert(dev);
	usb_mid_t *usb_mid = usb_device_data_get(dev);
	assert(usb_mid);

	usb_log_info("USB MID gone: `%s'.\n", usb_device_get_name(dev));

	/* Remove ctl function */
	errno_t ret = ddf_fun_unbind(usb_mid->ctl_fun);
	if (ret != EOK) {
		usb_log_error("Failed to unbind USB MID ctl function: %s.\n",
		    str_error(ret));
		return ret;
	}
	ddf_fun_destroy(usb_mid->ctl_fun);

	/* Now remove all other functions */
	while (!list_empty(&usb_mid->interface_list)) {
		link_t *item = list_first(&usb_mid->interface_list);
		list_remove(item);

		usbmid_interface_t *iface = usbmid_interface_from_link(item);

		usb_log_info("Child `%s' is gone.\n",
		    ddf_fun_get_name(iface->fun));

		const errno_t pret = usbmid_interface_destroy(iface);
		if (pret != EOK) {
			usb_log_error("Failed to remove child `%s': %s\n",
			    ddf_fun_get_name(iface->fun), str_error(pret));
			ret = pret;
		}
	}
	return ret;
}

/** USB MID driver ops. */
static const usb_driver_ops_t mid_driver_ops = {
	.device_add = usbmid_device_add,
	.device_rem = usbmid_device_remove,
	.device_gone = usbmid_device_gone,
};

/** USB MID driver. */
static const usb_driver_t mid_driver = {
	.name = NAME,
	.ops = &mid_driver_ops,
	.endpoints = NULL
};

int main(int argc, char *argv[])
{
	printf(NAME ": USB multi interface device driver.\n");

	log_init(NAME);

	return usb_driver_main(&mid_driver);
}

/**
 * @}
 */
