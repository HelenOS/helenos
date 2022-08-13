/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include "vhcd.h"
#include "hub/virthub.h"

static vhc_virtdev_t *vhc_virtdev_create(void)
{
	vhc_virtdev_t *dev = malloc(sizeof(vhc_virtdev_t));
	if (dev == NULL) {
		return NULL;
	}
	dev->address = 0;
	dev->dev_sess = NULL;
	dev->dev_local = NULL;
	dev->plugged = true;
	link_initialize(&dev->link);
	fibril_mutex_initialize(&dev->guard);
	list_initialize(&dev->transfer_queue);

	return dev;
}

static errno_t vhc_virtdev_plug_generic(vhc_data_t *vhc,
    async_sess_t *sess, usbvirt_device_t *virtdev,
    uintptr_t *handle, bool connect, usb_address_t address)
{
	vhc_virtdev_t *dev = vhc_virtdev_create();
	if (dev == NULL) {
		return ENOMEM;
	}

	dev->dev_sess = sess;
	dev->dev_local = virtdev;
	dev->address = address;

	fibril_mutex_lock(&vhc->guard);
	list_append(&dev->link, &vhc->devices);
	fibril_mutex_unlock(&vhc->guard);

	fid_t fibril = fibril_create(vhc_transfer_queue_processor, dev);
	if (fibril == 0) {
		free(dev);
		return ENOMEM;
	}
	fibril_add_ready(fibril);

	if (handle != NULL) {
		*handle = (uintptr_t) dev;
	}

	if (connect) {
		// FIXME: check status
		(void) virthub_connect_device(&vhc->hub, dev);
	}

	return EOK;
}

errno_t vhc_virtdev_plug(vhc_data_t *vhc, async_sess_t *sess, uintptr_t *handle)
{
	return vhc_virtdev_plug_generic(vhc, sess, NULL, handle, true, 0);
}

errno_t vhc_virtdev_plug_local(vhc_data_t *vhc, usbvirt_device_t *dev, uintptr_t *handle)
{
	return vhc_virtdev_plug_generic(vhc, NULL, dev, handle, true, 0);
}

errno_t vhc_virtdev_plug_hub(vhc_data_t *vhc, usbvirt_device_t *dev,
    uintptr_t *handle, usb_address_t address)
{
	return vhc_virtdev_plug_generic(vhc, NULL, dev, handle, false, address);
}

void vhc_virtdev_unplug(vhc_data_t *vhc, uintptr_t handle)
{
	vhc_virtdev_t *dev = (vhc_virtdev_t *) handle;

	// FIXME: check status
	(void) virthub_disconnect_device(&vhc->hub, dev);

	fibril_mutex_lock(&vhc->guard);
	fibril_mutex_lock(&dev->guard);
	dev->plugged = false;
	list_remove(&dev->link);
	fibril_mutex_unlock(&dev->guard);
	fibril_mutex_unlock(&vhc->guard);
}
