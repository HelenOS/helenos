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

/** @addtogroup libusbhid
 * @{
 */
/** @file
 * Client functions for accessing USB HID interface (implementation).
 */
#include <dev_iface.h>
#include <usbhid_iface.h>
#include <usb/hid/iface.h>
#include <errno.h>
#include <str_error.h>
#include <async.h>
#include <assert.h>

/** Ask for event array length.
 *
 * @param dev_phone Opened phone to DDF device providing USB HID interface.
 * @return Number of usages returned or negative error code.
 */
int usbhid_dev_get_event_length(int dev_phone, size_t *size)
{
	if (dev_phone < 0) {
		return EINVAL;
	}

	sysarg_t len;
	int rc = async_req_1_1(dev_phone, DEV_IFACE_ID(USBHID_DEV_IFACE),
	    IPC_M_USBHID_GET_EVENT_LENGTH, &len);
	if (rc == EOK) {
		*size = (size_t) len;
	}
	
	return rc;
}

/** Request for next event from HID device.
 *
 * @param[in] dev_phone Opened phone to DDF device providing USB HID interface.
 * @param[out] usage_pages Where to store usage pages.
 * @param[out] usages Where to store usages (actual data).
 * @param[in] usage_count Length of @p usage_pages and @p usages buffer
 *	(in items, not bytes).
 * @param[out] actual_usage_count Number of usages actually returned by the
 *	device driver.
 * @param[in] flags Flags (see USBHID_IFACE_FLAG_*).
 * @return Error code.
 */
int usbhid_dev_get_event(int dev_phone, uint8_t *buf, 
    size_t size, size_t *actual_size, unsigned int flags)
{
	if (dev_phone < 0) {
		return EINVAL;
	}
	if ((buf == NULL)) {
		return ENOMEM;
	}
	if (size == 0) {
		return EINVAL;
	}
	
//	if (size == 0) {
//		return EOK;
//	}

	size_t buffer_size =  size;
	uint8_t *buffer = malloc(buffer_size);
	if (buffer == NULL) {
		return ENOMEM;
	}

	aid_t opening_request = async_send_2(dev_phone,
	    DEV_IFACE_ID(USBHID_DEV_IFACE), IPC_M_USBHID_GET_EVENT,
	    flags, NULL);
	if (opening_request == 0) {
		free(buffer);
		return ENOMEM;
	}

	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(dev_phone, buffer, buffer_size,
	    &data_request_call);
	if (data_request == 0) {
		async_wait_for(opening_request, NULL);
		free(buffer);
		return ENOMEM;
	}

	sysarg_t data_request_rc;
	sysarg_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if (data_request_rc != EOK) {
		/* Prefer return code of the opening request. */
		if (opening_request_rc != EOK) {
			return (int) opening_request_rc;
		} else {
			return (int) data_request_rc;
		}
	}

	if (opening_request_rc != EOK) {
		return (int) opening_request_rc;
	}

	size_t act_size = IPC_GET_ARG2(data_request_call);

	/* Copy the individual items. */
	memcpy(buf, buffer, act_size);
//	memcpy(usages, buffer + items, items * sizeof(int32_t));

	if (actual_size != NULL) {
		*actual_size = act_size;
	}

	return EOK;
}


int usbhid_dev_get_report_descriptor_length(int dev_phone, size_t *size)
{
	/** @todo Implement! */
	return ENOTSUP;
}

int usbhid_dev_get_report_descriptor(int dev_phone, uint8_t *buf, size_t size, 
    size_t *actual_size)
{
	/** @todo Implement! */
	return ENOTSUP;
}

/**
 * @}
 */
