#include <errno.h>
#include "vhcd.h"
#include "hub/virthub.h"


static vhc_virtdev_t *vhc_virtdev_create()
{
	vhc_virtdev_t *dev = malloc(sizeof(vhc_virtdev_t));
	if (dev == NULL) {
		return NULL;
	}
	dev->address = 0;
	dev->dev_phone = -1;
	dev->dev_local = NULL;
	dev->plugged = true;
	link_initialize(&dev->link);
	fibril_mutex_initialize(&dev->guard);
	list_initialize(&dev->transfer_queue);

	return dev;
}

static int vhc_virtdev_plug_generic(vhc_data_t *vhc,
    int phone, usbvirt_device_t *virtdev,
    uintptr_t *handle, bool connect)
{
	vhc_virtdev_t *dev = vhc_virtdev_create();
	if (dev == NULL) {
		return ENOMEM;
	}

	dev->dev_phone = phone;
	dev->dev_local = virtdev;

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
		(void) virthub_connect_device(vhc->hub, dev);
	}

	return EOK;
}

int vhc_virtdev_plug(vhc_data_t *vhc, int phone, uintptr_t *handle)
{
	return vhc_virtdev_plug_generic(vhc, phone, NULL, handle, true);
}

int vhc_virtdev_plug_local(vhc_data_t *vhc, usbvirt_device_t *dev, uintptr_t *handle)
{
	return vhc_virtdev_plug_generic(vhc, -1, dev, handle, true);
}

int vhc_virtdev_plug_hub(vhc_data_t *vhc, usbvirt_device_t *dev, uintptr_t *handle)
{
	return vhc_virtdev_plug_generic(vhc, -1, dev, handle, false);
}

void vhc_virtdev_unplug(vhc_data_t *vhc, uintptr_t handle)
{
	vhc_virtdev_t *dev = (vhc_virtdev_t *) handle;

	// FIXME: check status
	(void) virthub_disconnect_device(vhc->hub, dev);

	fibril_mutex_lock(&vhc->guard);
	fibril_mutex_lock(&dev->guard);
	dev->plugged = false;
	list_remove(&dev->link);
	fibril_mutex_unlock(&dev->guard);
	fibril_mutex_unlock(&vhc->guard);
}
