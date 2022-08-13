/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * IPC wrappers, host controller side.
 */
#include <errno.h>
#include <str.h>
#include <stdio.h>
#include <assert.h>
#include <async.h>
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>
#include <usb/debug.h>

/** Send control read transfer to virtual USB device.
 *
 * @param sess Session to the virtual device.
 * @param ep Target endpoint number.
 * @param setup_buffer Setup buffer.
 * @param setup_buffer_size Setup buffer size in bytes.
 * @param data_buffer Data buffer (DATA stage of control transfer).
 * @param data_buffer_size Size of data buffer in bytes.
 * @param data_transferred_size Number of actually transferred bytes.
 *
 * @return Error code.
 *
 */
errno_t usbvirt_ipc_send_control_read(async_sess_t *sess, void *setup_buffer,
    size_t setup_buffer_size, void *data_buffer, size_t data_buffer_size,
    size_t *data_transferred_size)
{
	if (!sess)
		return EINVAL;

	if ((setup_buffer == NULL) || (setup_buffer_size == 0))
		return EINVAL;

	if ((data_buffer == NULL) || (data_buffer_size == 0))
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);

	aid_t opening_request = async_send_0(exch, IPC_M_USBVIRT_CONTROL_READ,
	    NULL);
	if (opening_request == 0) {
		async_exchange_end(exch);
		return ENOMEM;
	}

	errno_t rc = async_data_write_start(exch, setup_buffer,
	    setup_buffer_size);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(opening_request);
		return rc;
	}

	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(exch, data_buffer,
	    data_buffer_size, &data_request_call);

	async_exchange_end(exch);

	if (data_request == 0) {
		async_forget(opening_request);
		return ENOMEM;
	}

	errno_t data_request_rc;
	errno_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if (data_request_rc != EOK) {
		/* Prefer the return code of the opening request. */
		if (opening_request_rc != EOK)
			return (errno_t) opening_request_rc;
		else
			return (errno_t) data_request_rc;
	}

	if (opening_request_rc != EOK)
		return (errno_t) opening_request_rc;

	if (data_transferred_size != NULL)
		*data_transferred_size = ipc_get_arg2(&data_request_call);

	return EOK;
}

/** Send control write transfer to virtual USB device.
 *
 * @param sess Session to the virtual device.
 * @param ep Target endpoint number.
 * @param setup_buffer Setup buffer.
 * @param setup_buffer_size Setup buffer size in bytes.
 * @param data_buffer Data buffer (DATA stage of control transfer).
 * @param data_buffer_size Size of data buffer in bytes.
 *
 * @return Error code.
 *
 */
errno_t usbvirt_ipc_send_control_write(async_sess_t *sess, void *setup_buffer,
    size_t setup_buffer_size, void *data_buffer, size_t data_buffer_size)
{
	if (!sess)
		return EINVAL;

	if ((setup_buffer == NULL) || (setup_buffer_size == 0))
		return EINVAL;

	if ((data_buffer_size > 0) && (data_buffer == NULL))
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);

	aid_t opening_request = async_send_1(exch, IPC_M_USBVIRT_CONTROL_WRITE,
	    data_buffer_size, NULL);
	if (opening_request == 0) {
		async_exchange_end(exch);
		return ENOMEM;
	}

	errno_t rc = async_data_write_start(exch, setup_buffer,
	    setup_buffer_size);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(opening_request);
		return rc;
	}

	if (data_buffer_size > 0) {
		rc = async_data_write_start(exch, data_buffer,
		    data_buffer_size);
		if (rc != EOK) {
			async_exchange_end(exch);
			async_forget(opening_request);
			return rc;
		}
	}

	async_exchange_end(exch);

	errno_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	return (errno_t) opening_request_rc;
}

/** Request data transfer from virtual USB device.
 *
 * @param sess Session to the virtual device.
 * @param ep Target endpoint number.
 * @param tr_type Transfer type (interrupt or bulk).
 * @param data Data buffer.
 * @param data_size Size of the data buffer in bytes.
 * @param act_size Number of actually returned bytes.
 *
 * @return Error code.
 *
 */
errno_t usbvirt_ipc_send_data_in(async_sess_t *sess, usb_endpoint_t ep,
    usb_transfer_type_t tr_type, void *data, size_t data_size, size_t *act_size)
{
	if (!sess)
		return EINVAL;

	usbvirt_hc_to_device_method_t method;

	switch (tr_type) {
	case USB_TRANSFER_INTERRUPT:
		method = IPC_M_USBVIRT_INTERRUPT_IN;
		break;
	case USB_TRANSFER_BULK:
		method = IPC_M_USBVIRT_BULK_IN;
		break;
	default:
		return EINVAL;
	}

	if ((ep <= 0) || (ep >= USBVIRT_ENDPOINT_MAX))
		return EINVAL;

	if ((data == NULL) || (data_size == 0))
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);

	aid_t opening_request = async_send_2(exch, method, ep, tr_type, NULL);
	if (opening_request == 0) {
		async_exchange_end(exch);
		return ENOMEM;
	}

	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(exch, data, data_size,
	    &data_request_call);

	async_exchange_end(exch);

	if (data_request == 0) {
		async_forget(opening_request);
		return ENOMEM;
	}

	errno_t data_request_rc;
	errno_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if (data_request_rc != EOK) {
		/* Prefer the return code of the opening request. */
		if (opening_request_rc != EOK)
			return (errno_t) opening_request_rc;
		else
			return (errno_t) data_request_rc;
	}

	if (opening_request_rc != EOK)
		return (errno_t) opening_request_rc;

	if (act_size != NULL)
		*act_size = ipc_get_arg2(&data_request_call);

	return EOK;
}

/** Send data to virtual USB device.
 *
 * @param sess Session to the virtual device.
 * @param ep Target endpoint number.
 * @param tr_type Transfer type (interrupt or bulk).
 * @param data Data buffer.
 * @param data_size Size of the data buffer in bytes.
 *
 * @return Error code.
 *
 */
errno_t usbvirt_ipc_send_data_out(async_sess_t *sess, usb_endpoint_t ep,
    usb_transfer_type_t tr_type, void *data, size_t data_size)
{
	if (!sess)
		return EINVAL;

	usbvirt_hc_to_device_method_t method;

	switch (tr_type) {
	case USB_TRANSFER_INTERRUPT:
		method = IPC_M_USBVIRT_INTERRUPT_OUT;
		break;
	case USB_TRANSFER_BULK:
		method = IPC_M_USBVIRT_BULK_OUT;
		break;
	default:
		return EINVAL;
	}

	if ((ep <= 0) || (ep >= USBVIRT_ENDPOINT_MAX))
		return EINVAL;

	if ((data == NULL) || (data_size == 0))
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);

	aid_t opening_request = async_send_1(exch, method, ep, NULL);
	if (opening_request == 0) {
		async_exchange_end(exch);
		return ENOMEM;
	}

	errno_t rc = async_data_write_start(exch, data, data_size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(opening_request);
		return rc;
	}

	errno_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	return (errno_t) opening_request_rc;
}

/**
 * @}
 */
