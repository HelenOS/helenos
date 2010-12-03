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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief Common stuff for both HC driver and hub driver.
 */
#include <usb/hcdhubd.h>
#include <usb/devreq.h>
#include <usbhc_iface.h>
#include <usb/descriptor.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>
#include <str_error.h>
#include <usb/classes/hub.h>

#include "hcdhubd_private.h"

/** Callback when new device is detected and must be handled by this driver.
 *
 * @param dev New device.
 * @return Error code.
 */
static int add_device(device_t *dev) {
	bool is_hc = str_cmp(dev->name, USB_HUB_DEVICE_NAME) != 0;
	printf("%s: add_device(name=\"%s\")\n", hc_driver->name, dev->name);

	if (is_hc) {
		/*
		 * We are the HC itself.
		 */
		return usb_add_hc_device(dev);
	} else {
		/*
		 * We are some (maybe deeply nested) hub.
		 * Thus, assign our own operations and explore already
		 * connected devices.
		 */
		return usb_add_hub_device(dev);
	}
}

/** Operations for combined HC and HUB driver. */
static driver_ops_t hc_driver_generic_ops = {
	.add_device = add_device
};

/** Combined HC and HUB driver. */
static driver_t hc_driver_generic = {
	.driver_ops = &hc_driver_generic_ops
};

/** Main USB host controller driver routine.
 *
 * @see driver_main
 *
 * @param hc Host controller driver.
 * @return Error code.
 */
int usb_hcd_main(usb_hc_driver_t *hc) {
	hc_driver = hc;
	hc_driver_generic.name = hc->name;

	/*
	 * Run the device driver framework.
	 */
	return driver_main(&hc_driver_generic);
}

/** Add a root hub for given host controller.
 * This function shall be called only once for each host controller driven
 * by this driver.
 * It takes care of creating child device - hub - that will be driven by
 * this task.
 *
 * @param dev Host controller device.
 * @return Error code.
 */
int usb_hcd_add_root_hub(usb_hc_device_t *dev)
{
	char *id;
	int rc = asprintf(&id, "usb&hc=%s&hub", hc_driver->name);
	if (rc <= 0) {
		return rc;
	}

	rc = usb_hc_add_child_device(dev->generic, USB_HUB_DEVICE_NAME, id, true);
	if (rc != EOK) {
		free(id);
	}

	return rc;
}

/** Info about child device. */
struct child_device_info {
	device_t *parent;
	const char *name;
	const char *match_id;
};

/** Adds a child device fibril worker. */
static int fibril_add_child_device(void *arg)
{
	struct child_device_info *child_info
	    = (struct child_device_info *) arg;
	int rc;

	async_usleep(1000);

	device_t *child = create_device();
	match_id_t *match_id = NULL;

	if (child == NULL) {
		rc = ENOMEM;
		goto failure;
	}
	child->name = child_info->name;

	match_id = create_match_id();
	if (match_id == NULL) {
		rc = ENOMEM;
		goto failure;
	}
	match_id->id = child_info->match_id;
	match_id->score = 10;
	add_match_id(&child->match_ids, match_id);

	printf("%s: adding child device `%s' with match \"%s\"\n",
	    hc_driver->name, child->name, match_id->id);
	rc = child_device_register(child, child_info->parent);
	printf("%s: child device `%s' registration: %s\n",
	    hc_driver->name, child->name, str_error(rc));

	if (rc != EOK) {
		goto failure;
	}

	goto leave;

failure:
	if (child != NULL) {
		child->name = NULL;
		delete_device(child);
	}

	if (match_id != NULL) {
		match_id->id = NULL;
		delete_match_id(match_id);
	}

leave:
	free(arg);
	return EOK;
}

/** Adds a child.
 * Due to deadlock in devman when parent registers child that oughts to be
 * driven by the same task, the child adding is done in separate fibril.
 * Not optimal, but it works.
 * Update: not under all circumstances the new fibril is successful either.
 * Thus the last parameter to let the caller choose.
 *
 * @param parent Parent device.
 * @param name Device name.
 * @param match_id Match id.
 * @param create_fibril Whether to run the addition in new fibril.
 * @return Error code.
 */
int usb_hc_add_child_device(device_t *parent, const char *name,
    const char *match_id, bool create_fibril)
{
	printf("%s: about to add child device `%s' (%s)\n", hc_driver->name,
	    name, match_id);

	/*
	 * Seems that creating fibril which postpones the action
	 * is the best solution.
	 */
	create_fibril = true;

	struct child_device_info *child_info
	    = malloc(sizeof(struct child_device_info));

	child_info->parent = parent;
	child_info->name = name;
	child_info->match_id = match_id;

	if (create_fibril) {
		fid_t fibril = fibril_create(fibril_add_child_device, child_info);
		if (!fibril) {
			return ENOMEM;
		}
		fibril_add_ready(fibril);
	} else {
		fibril_add_child_device(child_info);
	}

	return EOK;
}

/** Tell USB address of given device.
 *
 * @param handle Devman handle of the device.
 * @return USB device address or error code.
 */
usb_address_t usb_get_address_by_handle(devman_handle_t handle)
{
	/* TODO: search list of attached devices. */
	return ENOENT;
}

/**
 * @}
 */
