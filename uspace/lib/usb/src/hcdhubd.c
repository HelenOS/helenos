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
#include <usb_iface.h>
#include <usb/descriptor.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>
#include <str_error.h>
#include <usb/classes/hub.h>

#include "hcdhubd_private.h"


static int usb_iface_get_hc_handle(device_t *dev, devman_handle_t *handle)
{
	assert(dev);
	assert(dev->parent != NULL);

	device_t *parent = dev->parent;

	if (parent->ops && parent->ops->interfaces[USB_DEV_IFACE]) {
		usb_iface_t *usb_iface
		    = (usb_iface_t *) parent->ops->interfaces[USB_DEV_IFACE];
		assert(usb_iface != NULL);
		if (usb_iface->get_hc_handle) {
			int rc = usb_iface->get_hc_handle(parent, handle);
			return rc;
		}
	}

	return ENOTSUP;
}

static usb_iface_t usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle
};

static device_ops_t child_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface
};

/** Callback when new device is detected and must be handled by this driver.
 *
 * @param dev New device.
 * @return Error code.
 */
static int add_device(device_t *dev) {
	return ENOTSUP;
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
int usb_hcd_add_root_hub(device_t *dev)
{
	char *id;
	int rc = asprintf(&id, "usb&hub");
	if (rc <= 0) {
		return rc;
	}

	rc = usb_hc_add_child_device(dev, USB_HUB_DEVICE_NAME, id, true);
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
static int fibril_add_child_device(void *arg) {
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
	child->parent = child_info->parent;
	child->ops = &child_ops;

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
		const char *match_id, bool create_fibril) {
	printf("%s: about to add child device `%s' (%s)\n", hc_driver->name,
			name, match_id);

	/*
	 * Seems that creating fibril which postpones the action
	 * is the best solution.
	 */
	create_fibril = true;

	struct child_device_info *child_info
			= malloc(sizeof (struct child_device_info));

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
usb_address_t usb_get_address_by_handle(devman_handle_t handle) {
	/* TODO: search list of attached devices. */
	return ENOENT;
}

usb_address_t usb_use_free_address(usb_hc_device_t * this_hcd) {
	//is there free address?
	link_t * addresses = &this_hcd->addresses;
	if (list_empty(addresses)) return -1;
	link_t * link_addr = addresses;
	bool found = false;
	usb_address_list_t * range = NULL;
	while (!found) {
		link_addr = link_addr->next;
		if (link_addr == addresses) return -2;
		range = list_get_instance(link_addr,
				usb_address_list_t, link);
		if (range->upper_bound - range->lower_bound > 0) {
			found = true;
		}
	}
	//now we have interval
	int result = range->lower_bound;
	++(range->lower_bound);
	if (range->upper_bound - range->lower_bound == 0) {
		list_remove(&range->link);
		free(range);
	}
	return result;
}

void usb_free_used_address(usb_hc_device_t * this_hcd, usb_address_t addr) {
	//check range
	if (addr < usb_lowest_address || addr > usb_highest_address)
		return;
	link_t * addresses = &this_hcd->addresses;
	link_t * link_addr = addresses;
	//find 'good' interval
	usb_address_list_t * found_range = NULL;
	bool found = false;
	while (!found) {
		link_addr = link_addr->next;
		if (link_addr == addresses) {
			found = true;
		} else {
			usb_address_list_t * range = list_get_instance(link_addr,
					usb_address_list_t, link);
			if (	(range->lower_bound - 1 == addr) ||
					(range->upper_bound == addr)) {
				found = true;
				found_range = range;
			}
			if (range->lower_bound - 1 > addr) {
				found = true;
			}

		}
	}
	if (found_range == NULL) {
		//no suitable range found
		usb_address_list_t * result_range =
				(usb_address_list_t*) malloc(sizeof (usb_address_list_t));
		result_range->lower_bound = addr;
		result_range->upper_bound = addr + 1;
		list_insert_before(&result_range->link, link_addr);
	} else {
		//we have good range
		if (found_range->lower_bound - 1 == addr) {
			--found_range->lower_bound;
		} else {
			//only one possible case
			++found_range->upper_bound;
			if (found_range->link.next != addresses) {
				usb_address_list_t * next_range =
						list_get_instance( &found_range->link.next,
						usb_address_list_t, link);
				//check neighbour range
				if (next_range->lower_bound == addr + 1) {
					//join ranges
					found_range->upper_bound = next_range->upper_bound;
					list_remove(&next_range->link);
					free(next_range);
				}
			}
		}
	}

}

/**
 * @}
 */
