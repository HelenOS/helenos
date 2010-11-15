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
#include "hcdhubd.h"
#include <driver.h>
#include <bool.h>
#include <errno.h>

/** List of handled host controllers. */
static LIST_INITIALIZE(hc_list);

/** Our HC driver. */
static usb_hc_driver_t *hc_driver = NULL;

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
	bool is_hc = true;

	if (is_hc) {
		/*
		 * We are the HC itself.
		 */
		usb_hc_device_t *hc_dev = malloc(sizeof(usb_hc_device_t));
		list_initialize(&hc_dev->link);

		hc_dev->generic = dev;
		int rc = hc_driver->add_hc(hc_dev);
		if (rc != EOK) {
			free(hc_dev);
			return rc;
		}

		add_device_to_class(dev, "usbhc");

		list_append(&hc_dev->link, &hc_list);

		return EOK;
	} else {
		/*
		 * We are some (probably deeply nested) hub.
		 * Thus, assign our own operations and explore already
		 * connected devices.
		 */
		return ENOTSUP;
	}
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
			usb_hcd_local_transfer_interrupt_in(hc, target,
			    change_bitmap, byte_length, &actual_size,
			    &handle);

			usb_hcd_local_wait_for(handle);

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
	/*
	 * Announce presence of child device.
	 */
	device_t *hub = NULL;
	match_id_t *match_id = NULL;
	int rc;

	hub = create_device();
	if (hub == NULL) {
		rc = ENOMEM;
		goto failure;
	}
	hub->name = "usbhub";

	match_id = create_match_id();
	if (match_id == NULL) {
		rc = ENOMEM;
		goto failure;
	}

	char *id;
	rc = asprintf(&id, "usb&hc=%s&hub", dev->generic->name);
	if (rc <= 0) {
		rc = ENOMEM;
		goto failure;
	}

	match_id->id = id;
	match_id->score = 10;

	add_match_id(&hub->match_ids, match_id);

	rc = child_device_register(hub, dev->generic);
	if (rc != EOK) {
		goto failure;
	}

	return EOK;

failure:
	if (hub != NULL) {
		hub->name = NULL;
		delete_device(hub);
	}
	delete_match_id(match_id);

	return rc;
}


/** Issue interrupt IN transfer to HC driven by current task.
 *
 * @warning The @p buffer and @p actual_size parameters shall not be
 * touched until this transfer is waited for by usb_hcd_local_wait_for().
 *
 * @param hc Host controller to handle the transfer.
 * @param target Target device address.
 * @param buffer Data buffer.
 * @param size Buffer size.
 * @param actual_size Size of actually transferred data.
 * @param handle Transfer handle.
 * @return Error code.
 */
int usb_hcd_local_transfer_interrupt_in(usb_hc_device_t *hc,
    usb_target_t target, void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	/*
	 * TODO: verify that given endpoint is of interrupt type and
	 * call hc->transfer_ops->transfer_in()
	 */
	return ENOTSUP;
}

/** Wait for transfer to complete.
 *
 * @param handle Transfer handle.
 * @return Error code.
 */
int usb_hcd_local_wait_for(usb_handle_t handle)
{
	return ENOTSUP;
}

/**
 * @}
 */
