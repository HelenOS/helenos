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
 * @brief HC driver and hub driver (implementation).
 */
#include <usb/hcdhubd.h>
#include <usb/devreq.h>
#include <usbhc_iface.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>

#define USB_HUB_DEVICE_NAME "usbhub"

/** List of handled host controllers. */
static LIST_INITIALIZE(hc_list);

/** Our HC driver. */
static usb_hc_driver_t *hc_driver = NULL;

static usbhc_iface_t usb_interface = {
	.interrupt_out = NULL,
	.interrupt_in = NULL
};

static device_ops_t usb_device_ops = {
	.interfaces[USBHC_DEV_IFACE] = &usb_interface
};

static void set_hub_address(usb_hc_device_t *hc, usb_address_t address);

/** Callback when new device is detected and must be handled by this driver.
 *
 * @param dev New device.
 * @return Error code.
 */
static int add_device(device_t *dev)
{
	/*
	 * FIXME: use some magic to determine whether hub or another HC
	 * was connected.
	 */
	bool is_hc = str_cmp(dev->name, USB_HUB_DEVICE_NAME) != 0;
	printf("%s: add_device(name=\"%s\")\n", hc_driver->name, dev->name);

	if (is_hc) {
		/*
		 * We are the HC itself.
		 */
		usb_hc_device_t *hc_dev = malloc(sizeof(usb_hc_device_t));
		list_initialize(&hc_dev->link);
		hc_dev->transfer_ops = NULL;

		hc_dev->generic = dev;
		dev->ops = &usb_device_ops;
		hc_dev->generic->driver_data = hc_dev;

		int rc = hc_driver->add_hc(hc_dev);
		if (rc != EOK) {
			free(hc_dev);
			return rc;
		}

		/*
		 * FIXME: The following line causes devman to hang.
		 * Will investigate later why.
		 */
		// add_device_to_class(dev, "usbhc");

		list_append(&hc_dev->link, &hc_list);

		return EOK;
	} else {
		usb_hc_device_t *hc = list_get_instance(hc_list.next, usb_hc_device_t, link);
		set_hub_address(hc, 5);

		/*
		 * We are some (probably deeply nested) hub.
		 * Thus, assign our own operations and explore already
		 * connected devices.
		 */

		return ENOTSUP;
	}
}

/** Sample usage of usb_hc_async functions.
 * This function sets hub address using standard SET_ADDRESS request.
 *
 * @warning This function shall be removed once you are familiar with
 * the usb_hc_ API.
 *
 * @param hc Host controller the hub belongs to.
 * @param address New hub address.
 */
static void set_hub_address(usb_hc_device_t *hc, usb_address_t address)
{
	printf("%s: setting hub address to %d\n", hc->generic->name, address);
	usb_target_t target = {0, 0};
	usb_handle_t handle;
	int rc;

	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 0,
		.request = USB_DEVREQ_SET_ADDRESS,
		.index = 0,
		.length = 0,
	};
	setup_packet.value = address;

	rc = usb_hc_async_control_write_setup(hc, target,
	    &setup_packet, sizeof(setup_packet), &handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_hc_async_wait_for(handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_hc_async_control_write_status(hc, target, &handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_hc_async_wait_for(handle);
	if (rc != EOK) {
		return;
	}

	printf("%s: hub address changed\n", hc->generic->name);
}

/** Check changes on all known hubs.
 */
static void check_hub_changes(void)
{
	/*
	 * Iterate through all HCs.
	 */
	link_t *link_hc;
	for (link_hc = hc_list.next;
	    link_hc != &hc_list;
	    link_hc = link_hc->next) {
		usb_hc_device_t *hc = list_get_instance(link_hc,
		    usb_hc_device_t, link);
		/*
		 * Iterate through all their hubs.
		 */
		link_t *link_hub;
		for (link_hub = hc->hubs.next;
		    link_hub != &hc->hubs;
		    link_hub = link_hub->next) {
			usb_hcd_hub_info_t *hub = list_get_instance(link_hub,
			    usb_hcd_hub_info_t, link);

			/*
			 * Check status change pipe of this hub.
			 */
			usb_target_t target = {
				.address = hub->device->address,
				.endpoint = 1
			};

			// FIXME: count properly
			size_t byte_length = (hub->port_count / 8) + 1;

			void *change_bitmap = malloc(byte_length);
			size_t actual_size;
			usb_handle_t handle;

			/*
			 * Send the request.
			 * FIXME: check returned value for possible errors
			 */
			usb_hc_async_interrupt_in(hc, target,
			    change_bitmap, byte_length, &actual_size,
			    &handle);

			usb_hc_async_wait_for(handle);

			/*
			 * TODO: handle the changes.
			 */
		}
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
int usb_hcd_main(usb_hc_driver_t *hc)
{
	hc_driver = hc;
	hc_driver_generic.name = hc->name;

	/*
	 * Launch here fibril that will periodically check all
	 * attached hubs for status change.
	 * WARN: This call will effectively do nothing.
	 */
	check_hub_changes();

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
	int rc = asprintf(&id, "usb&hc=%s&hub", dev->generic->name);
	if (rc <= 0) {
		return rc;
	}

	rc = usb_hc_add_child_device(dev->generic, USB_HUB_DEVICE_NAME, id);
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
	printf("adding child device with match \"%s\"\n", match_id->id);
	add_match_id(&child->match_ids, match_id);

	rc = child_device_register(child, child_info->parent);
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
	return rc;
}

/** Adds a child.
 * Due to deadlock in devman when parent registers child that oughts to be
 * driven by the same task, the child adding is done in separate fibril.
 * Not optimal, but it works.
 *
 * @param parent Parent device.
 * @param name Device name.
 * @param match_id Match id.
 * @return Error code.
 */
int usb_hc_add_child_device(device_t *parent, const char *name,
    const char *match_id)
{
	struct child_device_info *child_info
	    = malloc(sizeof(struct child_device_info));

	child_info->parent = parent;
	child_info->name = name;
	child_info->match_id = match_id;

	fid_t fibril = fibril_create(fibril_add_child_device, child_info);
	if (!fibril) {
		return ENOMEM;
	}
	fibril_add_ready(fibril);

	return EOK;
}

/**
 * @}
 */
