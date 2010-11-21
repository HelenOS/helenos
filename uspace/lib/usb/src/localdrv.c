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
 * @brief Driver communication for local drivers (hubs).
 */
#include <usb/hcdhubd.h>
#include <usbhc_iface.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>

/** Information about pending transaction on HC. */
typedef struct {
	/** Storage for actual number of bytes transferred. */
	size_t *size_transferred;

	/** Target device. */
	usb_hcd_attached_device_info_t *device;
	/** Target endpoint. */
	usb_hc_endpoint_info_t *endpoint;

	/** Guard for the condition variable. */
	fibril_mutex_t condvar_guard;
	/** Condition variable for waiting for done. */
	fibril_condvar_t condvar;

	/** Flag whether the transfer is done. */
	bool done;
} transfer_info_t;

/** Create new transfer info.
 *
 * @param device Attached device.
 * @param endpoint Endpoint.
 * @return Transfer info with pre-filled values.
 */
static transfer_info_t *transfer_info_create(
    usb_hcd_attached_device_info_t *device, usb_hc_endpoint_info_t *endpoint)
{
	transfer_info_t *transfer = malloc(sizeof(transfer_info_t));

	transfer->size_transferred = NULL;
	fibril_condvar_initialize(&transfer->condvar);
	fibril_mutex_initialize(&transfer->condvar_guard);
	transfer->done = false;

	transfer->device = device;
	transfer->endpoint = endpoint;

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

/** Marks given transfer as done.
 *
 * @param transfer Transfer to be completed.
 */
static void transfer_mark_complete(transfer_info_t *transfer)
{
	fibril_mutex_lock(&transfer->condvar_guard);
	transfer->done = true;
	fibril_condvar_signal(&transfer->condvar);
	fibril_mutex_unlock(&transfer->condvar_guard);
}

/** Callback for OUT transfers.
 *
 * @param hc Host controller that processed the transfer.
 * @param outcome Transfer outcome.
 * @param arg Custom argument.
 */
static void callback_out(usb_hc_device_t *hc,
    usb_transaction_outcome_t outcome, void *arg)
{
	transfer_info_t *transfer = (transfer_info_t *) arg;

	/*
	 * For out transfers, marking them complete is enough.
	 * No other processing is necessary.
	 */
	transfer_mark_complete(transfer);
}

/** Callback for IN transfers.
 *
 * @param hc Host controller that processed the transfer.
 * @param actual_size Number of actually transferred bytes.
 * @param outcome Transfer outcome.
 * @param arg Custom argument.
 */
static void callback_in(usb_hc_device_t *hc,
    size_t actual_size, usb_transaction_outcome_t outcome, void *arg)
{
	transfer_info_t *transfer = (transfer_info_t *) arg;

	/*
	 * Set the number of actually transferred bytes and
	 * mark the transfer as complete.
	 */
	if (transfer->size_transferred != NULL) {
		*transfer->size_transferred = actual_size;
	}

	transfer_mark_complete(transfer);
}

static int async_transfer_out(usb_hc_device_t *hc,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *data, size_t size,
    usb_handle_t *handle)
{
	if ((hc->transfer_ops == NULL)
	    || (hc->transfer_ops->transfer_out == NULL)) {
		return ENOTSUP;
	}

	/* This creation of the device on the fly is just a workaround. */

	transfer_info_t *transfer = transfer_info_create(
	    create_attached_device_info(target.address),
	    create_endpoint_info(target.endpoint,
		USB_DIRECTION_OUT, transfer_type));

	int rc = hc->transfer_ops->transfer_out(hc,
	    transfer->device, transfer->endpoint,
	    data, size,
	    callback_out, transfer);

	if (rc != EOK) {
		transfer_info_destroy(transfer);
		return rc;
	}

	*handle = (usb_handle_t)transfer;

	return EOK;
}

static int async_transfer_setup(usb_hc_device_t *hc,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *data, size_t size,
    usb_handle_t *handle)
{
	if ((hc->transfer_ops == NULL)
	    || (hc->transfer_ops->transfer_setup == NULL)) {
		return ENOTSUP;
	}

	/* This creation of the device on the fly is just a workaround. */

	transfer_info_t *transfer = transfer_info_create(
	    create_attached_device_info(target.address),
	    create_endpoint_info(target.endpoint,
		USB_DIRECTION_OUT, transfer_type));

	int rc = hc->transfer_ops->transfer_setup(hc,
	    transfer->device, transfer->endpoint,
	    data, size,
	    callback_out, transfer);

	if (rc != EOK) {
		transfer_info_destroy(transfer);
		return rc;
	}

	*handle = (usb_handle_t)transfer;

	return EOK;
}

static int async_transfer_in(usb_hc_device_t *hc, usb_target_t target,
    usb_transfer_type_t transfer_type,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	if ((hc->transfer_ops == NULL)
	    || (hc->transfer_ops->transfer_in == NULL)) {
		return ENOTSUP;
	}

	/* This creation of the device on the fly is just a workaround. */

	transfer_info_t *transfer = transfer_info_create(
	    create_attached_device_info(target.address),
	    create_endpoint_info(target.endpoint,
		USB_DIRECTION_IN, transfer_type));
	transfer->size_transferred = actual_size;

	int rc = hc->transfer_ops->transfer_in(hc,
	    transfer->device, transfer->endpoint,
	    buffer, size,
	    callback_in, transfer);

	if (rc != EOK) {
		transfer_info_destroy(transfer);
		return rc;
	}

	*handle = (usb_handle_t)transfer;

	return EOK;
}


/** Issue interrupt OUT transfer to HC driven by current task.
 *
 * @param hc Host controller to handle the transfer.
 * @param target Target device address.
 * @param buffer Data buffer.
 * @param size Buffer size.
 * @param handle Transfer handle.
 * @return Error code.
 */
int usb_hc_async_interrupt_out(usb_hc_device_t *hc, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return async_transfer_out(hc, target,
	    USB_TRANSFER_INTERRUPT, buffer, size, handle);
}


/** Issue interrupt IN transfer to HC driven by current task.
 *
 * @warning The @p buffer and @p actual_size parameters shall not be
 * touched until this transfer is waited for by usb_hc_async_wait_for().
 *
 * @param hc Host controller to handle the transfer.
 * @param target Target device address.
 * @param buffer Data buffer.
 * @param size Buffer size.
 * @param actual_size Size of actually transferred data.
 * @param handle Transfer handle.
 * @return Error code.
 */
int usb_hc_async_interrupt_in(usb_hc_device_t *hc, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	return async_transfer_in(hc, target,
	    USB_TRANSFER_INTERRUPT, buffer, size, actual_size, handle);
}

int usb_hc_async_control_write_setup(usb_hc_device_t *hc, usb_target_t target,
    void *data, size_t size, usb_handle_t *handle)
{
	return async_transfer_setup(hc, target,
	    USB_TRANSFER_CONTROL, data, size, handle);
}

int usb_hc_async_control_write_data(usb_hc_device_t *hc, usb_target_t target,
    void *data, size_t size, usb_handle_t *handle)
{
	return async_transfer_out(hc, target,
	    USB_TRANSFER_CONTROL, data, size, handle);
}

int usb_hc_async_control_write_status(usb_hc_device_t *hc, usb_target_t target,
    usb_handle_t *handle)
{
	return async_transfer_in(hc, target,
	    USB_TRANSFER_CONTROL, NULL, 0, NULL, handle);
}

int usb_hc_async_control_read_setup(usb_hc_device_t *hc, usb_target_t target,
    void *data, size_t size, usb_handle_t *handle)
{
	return async_transfer_setup(hc, target,
	    USB_TRANSFER_CONTROL, data, size, handle);
}

int usb_hc_async_control_read_data(usb_hc_device_t *hc, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	return async_transfer_in(hc, target,
	    USB_TRANSFER_CONTROL, buffer, size, actual_size, handle);
}

int usb_hc_async_control_read_status(usb_hc_device_t *hc, usb_target_t target,
    usb_handle_t *handle)
{
	return async_transfer_out(hc, target,
	    USB_TRANSFER_CONTROL, NULL, 0, handle);
}

/** Wait for transfer to complete.
 *
 * @param handle Transfer handle.
 * @return Error code.
 */
int usb_hc_async_wait_for(usb_handle_t handle)
{
	transfer_info_t *transfer = (transfer_info_t *) handle;
	if (transfer == NULL) {
		return ENOENT;
	}

	fibril_mutex_lock(&transfer->condvar_guard);
	while (!transfer->done) {
		fibril_condvar_wait(&transfer->condvar, &transfer->condvar_guard);
	}
	fibril_mutex_unlock(&transfer->condvar_guard);

	transfer_info_destroy(transfer);

	return EOK;
}

/**
 * @}
 */
