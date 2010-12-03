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
 * @brief Driver communication for remote drivers (interface implementation).
 */
#include <usb/hcdhubd.h>
#include <usbhc_iface.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>

#include "hcdhubd_private.h"

static int remote_get_address(device_t *, devman_handle_t, usb_address_t *);

static int remote_interrupt_out(device_t *, usb_target_t, void *, size_t,
    usbhc_iface_transfer_out_callback_t, void *);
static int remote_interrupt_in(device_t *, usb_target_t, void *, size_t,
    usbhc_iface_transfer_in_callback_t, void *);

static int remote_control_write_setup(device_t *, usb_target_t,
    void *, size_t,
    usbhc_iface_transfer_out_callback_t, void *);
static int remote_control_write_data(device_t *, usb_target_t,
    void *, size_t,
    usbhc_iface_transfer_out_callback_t, void *);
static int remote_control_write_status(device_t *, usb_target_t,
    usbhc_iface_transfer_in_callback_t, void *);

static int remote_control_read_setup(device_t *, usb_target_t,
    void *, size_t,
    usbhc_iface_transfer_out_callback_t, void *);
static int remote_control_read_data(device_t *, usb_target_t,
    void *, size_t,
    usbhc_iface_transfer_in_callback_t, void *);
static int remote_control_read_status(device_t *, usb_target_t,
    usbhc_iface_transfer_out_callback_t, void *);

/** Implementation of USB HC interface. */
usbhc_iface_t usbhc_interface = {
	.tell_address = remote_get_address,
	.interrupt_out = remote_interrupt_out,
	.interrupt_in = remote_interrupt_in,
	.control_write_setup = remote_control_write_setup,
	.control_write_data = remote_control_write_data,
	.control_write_status = remote_control_write_status,
	.control_read_setup = remote_control_read_setup,
	.control_read_data = remote_control_read_data,
	.control_read_status = remote_control_read_status
};

/** Get USB address for remote USBHC interface.
 *
 * @param dev Device asked for the information.
 * @param handle Devman handle of the USB device.
 * @param address Storage for obtained address.
 * @return Error code.
 */
int remote_get_address(device_t *dev, devman_handle_t handle,
    usb_address_t *address)
{
	usb_address_t addr = usb_get_address_by_handle(handle);
	if (addr < 0) {
		return addr;
	}

	*address = addr;

	return EOK;
}

/** Information about pending transaction on HC. */
typedef struct {
	/** Target device. */
	usb_hcd_attached_device_info_t *device;
	/** Target endpoint. */
	usb_hc_endpoint_info_t *endpoint;

	/** Callbacks. */
	union {
		/** Callback for outgoing transfers. */
		usbhc_iface_transfer_out_callback_t out_callback;
		/** Callback for incoming transfers. */
		usbhc_iface_transfer_in_callback_t in_callback;
	};

	/** Custom argument for the callback. */
	void *arg;
} transfer_info_t;

/** Create new transfer info.
 *
 * @param device Attached device.
 * @param endpoint Endpoint.
 * @param custom_arg Custom argument.
 * @return Transfer info with pre-filled values.
 */
static transfer_info_t *transfer_info_create(
    usb_hcd_attached_device_info_t *device, usb_hc_endpoint_info_t *endpoint,
    void *custom_arg)
{
	transfer_info_t *transfer = malloc(sizeof(transfer_info_t));

	transfer->device = device;
	transfer->endpoint = endpoint;
	transfer->arg = custom_arg;
	transfer->out_callback = NULL;
	transfer->in_callback = NULL;

	return transfer;
}

/** Destroy transfer info.
 *
 * @param transfer Transfer to be destroyed.
 */
static void transfer_info_destroy(transfer_info_t *transfer)
{
	free(transfer->device);
	free(transfer->endpoint);
	free(transfer);
}

/** Create info about attached device.
 *
 * @param address Device address.
 * @return Device info structure.
 */
static usb_hcd_attached_device_info_t *create_attached_device_info(
    usb_address_t address)
{
	usb_hcd_attached_device_info_t *dev
	    = malloc(sizeof(usb_hcd_attached_device_info_t));

	dev->address = address;
	dev->endpoint_count = 0;
	dev->endpoints = NULL;
	list_initialize(&dev->link);

	return dev;
}

/** Create info about device endpoint.
 *
 * @param endpoint Endpoint number.
 * @param direction Endpoint data direction.
 * @param transfer_type Transfer type of the endpoint.
 * @return Endpoint info structure.
 */
static usb_hc_endpoint_info_t *create_endpoint_info(usb_endpoint_t endpoint,
    usb_direction_t direction, usb_transfer_type_t transfer_type)
{
	usb_hc_endpoint_info_t *ep = malloc(sizeof(usb_hc_endpoint_info_t));
	ep->data_toggle = 0;
	ep->direction = direction;
	ep->transfer_type = transfer_type;
	ep->endpoint = endpoint;

	return ep;
}



/** Callback for OUT transfers.
 * This callback is called by implementation of HC operations.
 *
 * @param hc Host controller that processed the transfer.
 * @param outcome Transfer outcome.
 * @param arg Custom argument.
 */
static void remote_out_callback(usb_hc_device_t *hc,
    usb_transaction_outcome_t outcome, void *arg)
{
	transfer_info_t *transfer = (transfer_info_t *) arg;
	transfer->out_callback(hc->generic, outcome, transfer->arg);

	transfer_info_destroy(transfer);
}

/** Start an OUT transfer.
 *
 * @param dev Device that shall process the transfer.
 * @param target Target device for the data.
 * @param transfer_type Transfer type.
 * @param data Data buffer.
 * @param size Size of data buffer.
 * @param callback Callback after transfer is complete.
 * @param arg Custom argument to the callback.
 * @return Error code.
 */
static int remote_out_transfer(device_t *dev, usb_target_t target,
    usb_transfer_type_t transfer_type, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_hc_device_t *hc = (usb_hc_device_t *) dev->driver_data;

	if ((hc->transfer_ops == NULL)
	    || (hc->transfer_ops->transfer_out == NULL)) {
		return ENOTSUP;
	}

	transfer_info_t *transfer = transfer_info_create(
	    create_attached_device_info(target.address),
	    create_endpoint_info(target.endpoint,
		USB_DIRECTION_OUT, transfer_type),
	    arg);
	transfer->out_callback = callback;

	int rc = hc->transfer_ops->transfer_out(hc,
	    transfer->device, transfer->endpoint,
	    data, size,
	    remote_out_callback, transfer);

	if (rc != EOK) {
		transfer_info_destroy(transfer);
		return rc;
	}

	return EOK;
}

/** Start a SETUP transfer.
 *
 * @param dev Device that shall process the transfer.
 * @param target Target device for the data.
 * @param transfer_type Transfer type.
 * @param data Data buffer.
 * @param size Size of data buffer.
 * @param callback Callback after transfer is complete.
 * @param arg Custom argument to the callback.
 * @return Error code.
 */
static int remote_setup_transfer(device_t *dev, usb_target_t target,
    usb_transfer_type_t transfer_type, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_hc_device_t *hc = (usb_hc_device_t *) dev->driver_data;

	if ((hc->transfer_ops == NULL)
	    || (hc->transfer_ops->transfer_setup == NULL)) {
		return ENOTSUP;
	}

	transfer_info_t *transfer = transfer_info_create(
	    create_attached_device_info(target.address),
	    create_endpoint_info(target.endpoint,
		USB_DIRECTION_OUT, transfer_type),
	    arg);
	transfer->out_callback = callback;

	int rc = hc->transfer_ops->transfer_setup(hc,
	    transfer->device, transfer->endpoint,
	    data, size,
	    remote_out_callback, transfer);

	if (rc != EOK) {
		transfer_info_destroy(transfer);
		return rc;
	}

	return EOK;
}

/** Callback for IN transfers.
 * This callback is called by implementation of HC operations.
 *
 * @param hc Host controller that processed the transfer.
 * @param outcome Transfer outcome.
 * @param actual_size Size of actually received data.
 * @param arg Custom argument.
 */
static void remote_in_callback(usb_hc_device_t *hc,
    usb_transaction_outcome_t outcome, size_t actual_size, void *arg)
{
	transfer_info_t *transfer = (transfer_info_t *) arg;
	transfer->in_callback(hc->generic, outcome, actual_size, transfer->arg);

	transfer_info_destroy(transfer);
}

/** Start an IN transfer.
 *
 * @param dev Device that shall process the transfer.
 * @param target Target device for the data.
 * @param transfer_type Transfer type.
 * @param data Data buffer.
 * @param size Size of data buffer.
 * @param callback Callback after transfer is complete.
 * @param arg Custom argument to the callback.
 * @return Error code.
 */
static int remote_in_transfer(device_t *dev, usb_target_t target,
    usb_transfer_type_t transfer_type, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	usb_hc_device_t *hc = (usb_hc_device_t *) dev->driver_data;

	if ((hc->transfer_ops == NULL)
	    || (hc->transfer_ops->transfer_in == NULL)) {
		return ENOTSUP;
	}

	transfer_info_t *transfer = transfer_info_create(
	    create_attached_device_info(target.address),
	    create_endpoint_info(target.endpoint,
		USB_DIRECTION_OUT, transfer_type),
	    arg);
	transfer->in_callback = callback;

	int rc = hc->transfer_ops->transfer_in(hc,
	    transfer->device, transfer->endpoint,
	    data, size,
	    remote_in_callback, transfer);

	if (rc != EOK) {
		transfer_info_destroy(transfer);
		return rc;
	}

	return EOK;
}

/** Start outgoing interrupt transfer (USBHC remote interface).
 *
 * @param dev Host controller device processing the transfer.
 * @param target Target USB device.
 * @param buffer Data buffer.
 * @param size Data buffer size.
 * @param callback Callback after the transfer is completed.
 * @param arg Custom argument to the callback.
 * @return Error code.
 */
int remote_interrupt_out(device_t *dev, usb_target_t target,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return remote_out_transfer(dev, target, USB_TRANSFER_INTERRUPT,
	    buffer, size, callback, arg);
}

/** Start incoming interrupt transfer (USBHC remote interface).
 *
 * @param dev Host controller device processing the transfer.
 * @param target Target USB device.
 * @param buffer Data buffer.
 * @param size Data buffer size.
 * @param callback Callback after the transfer is completed.
 * @param arg Custom argument to the callback.
 * @return Error code.
 */
int remote_interrupt_in(device_t *dev, usb_target_t target,
    void *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return remote_in_transfer(dev, target, USB_TRANSFER_INTERRUPT,
	    buffer, size, callback, arg);
}


int remote_control_write_setup(device_t *device, usb_target_t target,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return remote_setup_transfer(device, target, USB_TRANSFER_CONTROL,
	    buffer, size, callback, arg);
}

int remote_control_write_data(device_t *device, usb_target_t target,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return remote_out_transfer(device, target, USB_TRANSFER_CONTROL,
	    buffer, size, callback, arg);
}

int remote_control_write_status(device_t *device, usb_target_t target,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return remote_in_transfer(device, target, USB_TRANSFER_CONTROL,
	    NULL, 0, callback, arg);
}

int remote_control_read_setup(device_t *device, usb_target_t target,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return remote_setup_transfer(device, target, USB_TRANSFER_CONTROL,
	    buffer, size, callback, arg);
}

int remote_control_read_data(device_t *dev, usb_target_t target,
    void *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return remote_in_transfer(dev, target, USB_TRANSFER_CONTROL,
	    buffer, size, callback, arg);
}

int remote_control_read_status(device_t *device, usb_target_t target,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return remote_out_transfer(device, target, USB_TRANSFER_CONTROL,
	    NULL, 0, callback, arg);
}

/**
 * @}
 */
