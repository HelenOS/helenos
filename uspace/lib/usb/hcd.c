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
 * @brief USB Host Controller Driver (implementation).
 */
#include "hcd.h"
#include <devmap.h>
#include <fcntl.h>
#include <vfs/vfs.h>
#include <errno.h>


#define NAMESPACE "usb"

/** String representation for USB transfer type. */
const char * usb_str_transfer_type(usb_transfer_type_t t)
{
	switch (t) {
		case USB_TRANSFER_ISOCHRONOUS:
			return "isochronous";
		case USB_TRANSFER_INTERRUPT:
			return "interrupt";
		case USB_TRANSFER_CONTROL:
			return "control";
		case USB_TRANSFER_BULK:
			return "bulk";
		default:
			return "unknown";
	}
}

/** String representation of USB transaction outcome. */
const char * usb_str_transaction_outcome(usb_transaction_outcome_t o)
{
	switch (o) {
		case USB_OUTCOME_OK:
			return "ok";
		case USB_OUTCOME_CRCERROR:
			return "CRC error";
		case USB_OUTCOME_BABBLE:
			return "babble";
		default:
			return "unknown";
	}
}

/** Create necessary phones for comunicating with HCD.
 * This function wraps following calls:
 * -# open <code>/dev/usb/<i>hcd_path</i></code> for reading
 * -# access phone of file opened in previous step
 * -# create callback through just opened phone
 * -# set handler for this callback
 * -# return the (outgoing) phone
 *
 * @warning This function is wrapper for several actions and therefore
 * it is not possible - in case of error - to determine at which point
 * error occured.
 *
 * @param hcd_path HCD identification under devfs
 *     (without <code>/dev/usb/</code>).
 * @param callback_connection Handler for callbacks from HCD.
 * @return Phone for comunicating with HCD or error code from errno.h.
 */
int usb_hcd_create_phones(const char * hcd_path,
    async_client_conn_t callback_connection)
{
	char dev_path[DEVMAP_NAME_MAXLEN + 1];
	snprintf(dev_path, DEVMAP_NAME_MAXLEN,
	    "/dev/%s/%s", NAMESPACE, hcd_path);
	
	int fd = open(dev_path, O_RDONLY);
	if (fd < 0) {
		return fd;
	}
	
	int hcd_phone = fd_phone(fd);
	
	if (hcd_phone < 0) {
		return hcd_phone;
	}
	
	ipcarg_t phonehash;
	int rc = ipc_connect_to_me(hcd_phone, 0, 0, 0, &phonehash);
	if (rc != EOK) {
		return rc;
	}
	async_new_connection(phonehash, 0, NULL, callback_connection);
	
	return hcd_phone;
}

/** Send data from USB host to a function.
 *
 * @param hcd_phone Connected phone to HCD.
 * @param target USB function address.
 * @param transfer_type USB transfer type.
 * @param buffer Buffer with data to be sent.
 * @param len Buffer @p buffer size.
 * @param[out] transaction_handle Handle of created transaction (NULL to ignore).
 * @return Error status.
 * @retval EOK Everything OK, buffer transfered to HCD and queued there.
 * @retval EINVAL Invalid phone.
 * @retval EINVAL @p buffer is NULL.
 */
int usb_hcd_send_data_to_function(int hcd_phone,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void * buffer, size_t len,
    usb_transaction_handle_t * transaction_handle)
{
	if (hcd_phone < 0) {
		return EINVAL;
	}
	if (buffer == NULL) {
		return EINVAL;
	}

	ipc_call_t answer_data;
	ipcarg_t answer_rc;
	aid_t req;
	int rc;
	
	req = async_send_4(hcd_phone,
	    IPC_M_USB_HCD_SEND_DATA,
	    target.address, target.endpoint,
	    transfer_type, 0,
	    &answer_data);
	
	rc = async_data_write_start(hcd_phone, buffer, len);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return rc;
	}
	
	async_wait_for(req, &answer_rc);
	rc = (int)answer_rc;
	if (rc != EOK) {
		return rc;
	}
	
	if (transaction_handle != NULL) {
		*transaction_handle = IPC_GET_ARG1(answer_data);
	}
	
	return EOK;
}


/** Inform HCD about data reception.
 * The actual reception is handled in callback.
 *
 * @param hcd_phone Connected phone to HCD.
 * @param target USB function address.
 * @param transfer_type USB transfer type.
 * @param len Maximum accepted packet size.
 * @param[out] transaction_handle Handle of created transaction (NULL to ignore).
 * @return Error status.
 */
int usb_hcd_prepare_data_reception(int hcd_phone,
    usb_target_t target, usb_transfer_type_t transfer_type,
    size_t len,
    usb_transaction_handle_t * transaction_handle)
{
	if (hcd_phone < 0) {
		return EINVAL;
	}
	
	usb_transaction_handle_t handle;
	
	int rc = ipc_call_sync_5_1(hcd_phone, IPC_M_USB_HCD_RECEIVE_DATA,
	    target.address, target.endpoint,
	    transfer_type, len, 0, &handle);
	
	if (rc != EOK) {
		return rc;
	}
	
	if (transaction_handle != NULL) {
		*transaction_handle = handle;
	}
	
	return EOK;
}


static int send_buffer(int phone, ipcarg_t method, usb_target_t target,
    void *buffer, size_t size, usb_transaction_handle_t * transaction_handle)
{
	if (phone < 0) {
		return EINVAL;
	}
	
	if ((buffer == NULL) && (size > 0)) {
		return EINVAL;
	}

	ipc_call_t answer_data;
	ipcarg_t answer_rc;
	aid_t req;
	int rc;
	
	req = async_send_3(phone,
	    method,
	    target.address, target.endpoint,
	    size,
	    &answer_data);
	
	if (size > 0) {
		rc = async_data_write_start(phone, buffer, size);
		if (rc != EOK) {
			async_wait_for(req, NULL);
			return rc;
		}
	}
	
	async_wait_for(req, &answer_rc);
	rc = (int)answer_rc;
	if (rc != EOK) {
		return rc;
	}
	
	if (transaction_handle != NULL) {
		*transaction_handle = IPC_GET_ARG1(answer_data);
	}
	
	return EOK;
}


static int prep_receive_data(int phone, ipcarg_t method, usb_target_t target,
    size_t size, usb_transaction_handle_t * transaction_handle)
{
	if (phone < 0) {
		return EINVAL;
	}
	
	usb_transaction_handle_t handle;
	
	int rc = ipc_call_sync_3_1(phone,
	    method,
	    target.address, target.endpoint,
	    size,
	    &handle);
	
	if (rc != EOK) {
		return rc;
	}
	
	if (transaction_handle != NULL) {
		*transaction_handle = handle;
	}
	
	return EOK;
}


int usb_hcd_transfer_interrupt_out(int hcd_phone, usb_target_t target,
    void *buffer, size_t size, usb_transaction_handle_t *handle)
{
	return send_buffer(hcd_phone, IPC_M_USB_HCD_INTERRUPT_OUT,
	    target, buffer, size, handle);
}

int usb_hcd_transfer_interrupt_in(int hcd_phone, usb_target_t target,
    size_t size, usb_transaction_handle_t *handle)
{
	return prep_receive_data(hcd_phone, IPC_M_USB_HCD_INTERRUPT_IN,
	    target, size, handle);
}

int usb_hcd_transfer_control_write_setup(int hcd_phone, usb_target_t target,
    void *buffer, size_t size, usb_transaction_handle_t *handle)
{
	return send_buffer(hcd_phone, IPC_M_USB_HCD_CONTROL_WRITE_SETUP,
	    target, buffer, size, handle);
}

int usb_hcd_transfer_control_write_data(int hcd_phone, usb_target_t target,
    void *buffer, size_t size, usb_transaction_handle_t *handle)
{
	return send_buffer(hcd_phone, IPC_M_USB_HCD_CONTROL_WRITE_DATA,
	    target, buffer, size, handle);
	
}
int usb_hcd_transfer_control_write_status(int hcd_phone, usb_target_t target,
    usb_transaction_handle_t *handle)
{
	return prep_receive_data(hcd_phone, IPC_M_USB_HCD_CONTROL_WRITE_STATUS,
	    target, 0, handle);
}

int usb_hcd_transfer_control_read_setup(int hcd_phone, usb_target_t target,
    void *buffer, size_t size, usb_transaction_handle_t *handle)
{
	return send_buffer(hcd_phone, IPC_M_USB_HCD_CONTROL_READ_SETUP,
	    target, buffer, size, handle);
}
int usb_hcd_transfer_control_read_data(int hcd_phone, usb_target_t target,
    size_t size, usb_transaction_handle_t *handle)
{
	return prep_receive_data(hcd_phone, IPC_M_USB_HCD_CONTROL_READ_DATA,
	   target, size, handle);
}
int usb_hcd_transfer_control_read_status(int hcd_phone, usb_target_t target,
    usb_transaction_handle_t *handle)
{
	return send_buffer(hcd_phone, IPC_M_USB_HCD_CONTROL_READ_STATUS,
	    target, NULL, 0, handle);
}



/**
 * @}
 */
