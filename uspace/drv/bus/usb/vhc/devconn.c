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
