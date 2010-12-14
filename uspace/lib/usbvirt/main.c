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

/** @addtogroup libusbvirt usb
 * @{
 */
/** @file
 * @brief Device registration with virtual USB framework.
 */
#include <devman.h>
#include <errno.h>
#include <stdlib.h>
#include <mem.h>
#include <assert.h>

#include "hub.h"
#include "device.h"
#include "private.h"

#define NAMESPACE "usb"

/** Virtual device wrapper. */
typedef struct {
	/** Actual device. */
	usbvirt_device_t *device;
	/** Phone to host controller. */
	int vhcd_phone;
	/** Device id. */
	sysarg_t id;
	/** Linked-list member. */
	link_t link;
} virtual_device_t;

/*** List of known device. */
static LIST_INITIALIZE(device_list);

/** Find virtual device wrapper based on the contents. */
static virtual_device_t *find_device(usbvirt_device_t *device)
{
	if (list_empty(&device_list)) {
		return NULL;
	}
	
	link_t *pos;
	for (pos = device_list.next; pos != &device_list; pos = pos->next) {
		virtual_device_t *dev
		    = list_get_instance(pos, virtual_device_t, link);
		if (dev->device == device) {
			return dev;
		}
	}
	
	return NULL;
}

/** Find virtual device wrapper by its id. */
static virtual_device_t *find_device_by_id(sysarg_t id)
{
	if (list_empty(&device_list)) {
		return NULL;
	}
	
	link_t *pos;
	for (pos = device_list.next; pos != &device_list; pos = pos->next) {
		virtual_device_t *dev
		    = list_get_instance(pos, virtual_device_t, link);
		if (dev->id == id) {
			return dev;
		}
	}
	
	return NULL;
}

/** Reply to a control transfer. */
static int control_transfer_reply(usbvirt_device_t *device,
	    usb_endpoint_t endpoint, void *buffer, size_t size)
{
	usbvirt_control_transfer_t *transfer = &device->current_control_transfers[endpoint];
	if (transfer->data != NULL) {
		free(transfer->data);
	}
	transfer->data = malloc(size);
	memcpy(transfer->data, buffer, size);
	transfer->data_size = size;
	
	return EOK;
}

/** Initialize virtual device. */
static void device_init(usbvirt_device_t *dev)
{
	dev->transaction_out = transaction_out;
	dev->transaction_setup = transaction_setup;
	dev->transaction_in = transaction_in;
	
	dev->control_transfer_reply = control_transfer_reply;
	
	dev->debug = user_debug;
	dev->lib_debug = lib_debug;
	
	dev->state = USBVIRT_STATE_DEFAULT;
	dev->address = 0;
	dev->new_address = -1;
	
	size_t i;
	for (i = 0; i < USB11_ENDPOINT_MAX; i++) {
		usbvirt_control_transfer_t *transfer = &dev->current_control_transfers[i];
		transfer->direction = 0;
		transfer->request = NULL;
		transfer->request_size = 0;
		transfer->data = NULL;
		transfer->data_size = 0;
	}
}

/** Add a virtual device.
 * The returned device (if not NULL) shall be destroy via destroy_device().
 */
static virtual_device_t *add_device(usbvirt_device_t *dev)
{
	assert(find_device(dev) == NULL);
	virtual_device_t *new_device
	    = (virtual_device_t *) malloc(sizeof(virtual_device_t));
	
	new_device->device = dev;
	link_initialize(&new_device->link);
	
	list_append(&new_device->link, &device_list);
	
	return new_device;
}

/** Destroy virtual device. */
static void destroy_device(virtual_device_t *dev)
{
	if (dev->vhcd_phone > 0) {
		ipc_hangup(dev->vhcd_phone);
	}
	
	list_remove(&dev->link);
	
	free(dev);
}

/** Callback connection handler. */
static void callback_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	// FIXME - determine which device just called back
	virtual_device_t *dev = find_device_by_id(0);
	if (dev == NULL) {
		ipc_answer_0(iid, EINVAL);
		printf("Ooops\n");
		return;
	}

	device_callback_connection(dev->device, iid, icall);
}

/** Create necessary phones for communication with virtual HCD.
 * This function wraps following calls:
 * -# open <code>/dev/devices/\\virt\\usbhc for reading
 * -# access phone of file opened in previous step
 * -# create callback through just opened phone
 * -# create handler for calling on data from host to function
 * -# return the (outgoing) phone
 *
 * @warning This function is wrapper for several actions and therefore
 * it is not possible - in case of error - to determine at which point
 * error occurred.
 *
 * @param dev Device to connect.
 * @return EOK on success or error code from errno.h.
 */
int usbvirt_connect(usbvirt_device_t *dev)
{
	virtual_device_t *virtual_device = find_device(dev);
	if (virtual_device != NULL) {
		return EEXISTS;
	}
	
	const char *vhc_path = "/virt/usbhc";
	int rc;
	devman_handle_t handle;

	rc = devman_device_get_handle(vhc_path, &handle, 0);
	if (rc != EOK) {
		printf("devman_device_get_handle() failed\n");
		return rc;
	}
	
	int hcd_phone = devman_device_connect(handle, 0);
	
	if (hcd_phone < 0) {
		printf("devman_device_connect() failed\n");
		return hcd_phone;
	}
	
	sysarg_t phonehash;
	rc = ipc_connect_to_me(hcd_phone, 0, 0, 0, &phonehash);
	if (rc != EOK) {
		printf("ipc_connect_to_me() failed\n");
		return rc;
	}
	
	device_init(dev);
	
	virtual_device = add_device(dev);
	virtual_device->vhcd_phone = hcd_phone;
	virtual_device->id = 0;
	
	async_new_connection(phonehash, 0, NULL, callback_connection);
	
	return EOK;
}

/** Prepares device as local.
 * This is useful if you want to have a virtual device in the same task
 * as HCD.
 *
 * @param dev Device to connect.
 * @return Error code.
 * @retval EOK Device connected.
 * @retval EEXISTS This device is already connected.
 */
int usbvirt_connect_local(usbvirt_device_t *dev)
{
	virtual_device_t *virtual_device = find_device(dev);
	if (virtual_device != NULL) {
		return EEXISTS;
	}
	
	device_init(dev);
	
	virtual_device = add_device(dev);
	virtual_device->vhcd_phone = -1;
	virtual_device->id = 0;
	
	return EOK;
}

/** Disconnects device from HCD.
 *
 * @param dev Device to be disconnected.
 * @return Error code.
 * @retval EOK Device connected.
 * @retval ENOENT This device is not connected.
 */
int usbvirt_disconnect(usbvirt_device_t *dev)
{
	virtual_device_t *virtual_device = find_device(dev);
	if (virtual_device == NULL) {
		return ENOENT;
	}
	
	destroy_device(virtual_device);
	
	return EOK;
}


/**
 * @}
 */
