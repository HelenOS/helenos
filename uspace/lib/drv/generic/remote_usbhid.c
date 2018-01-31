/*
 * Copyright (c) 2010-2011 Vojtech Horky
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <async.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <macros.h>

#include "usbhid_iface.h"
#include "ddf/driver.h"

/** IPC methods for USB HID device interface. */
typedef enum {
	/** Get number of events reported in single burst.
	 * Parameters: none
	 * Answer:
	 * - Size of one report in bytes.
	 */
	IPC_M_USBHID_GET_EVENT_LENGTH,
	/** Get single event from the HID device.
	 * The word single refers to set of individual events that were
	 * available at particular point in time.
	 * Parameters:
	 * - flags
	 * The call is followed by data read expecting two concatenated
	 * arrays.
	 * Answer:
	 * - EOK - events returned
	 * - EAGAIN - no event ready (only in non-blocking mode)
	 *
	 * It is okay if the client requests less data. Extra data must
	 * be truncated by the driver.
	 *
	 * @todo Change this comment.
	 */
	IPC_M_USBHID_GET_EVENT,
	
	/** Get the size of the report descriptor from the HID device.
	 *
	 * Parameters:
	 * - none
	 * Answer:
	 * - EOK - method is implemented (expected always)
	 * Parameters of the answer:
	 * - Size of the report in bytes.
	 */
	IPC_M_USBHID_GET_REPORT_DESCRIPTOR_LENGTH,
	
	/** Get the report descriptor from the HID device.
	 *
	 * Parameters:
	 * - none
	 * The call is followed by data read expecting the descriptor itself.
	 * Answer:
	 * - EOK - report descriptor returned.
	 */
	IPC_M_USBHID_GET_REPORT_DESCRIPTOR
} usbhid_iface_funcs_t;

/** Ask for event array length.
 *
 * @param dev_sess Session to DDF device providing USB HID interface.
 *
 * @return Number of usages returned or an error code.
 *
 */
errno_t usbhid_dev_get_event_length(async_sess_t *dev_sess, size_t *size)
{
	if (!dev_sess)
		return EINVAL;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	sysarg_t len;
	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(USBHID_DEV_IFACE),
	    IPC_M_USBHID_GET_EVENT_LENGTH, &len);
	
	async_exchange_end(exch);
	
	if (rc == EOK) {
		if (size != NULL)
			*size = (size_t) len;
	}
	
	return rc;
}

/** Request for next event from HID device.
 *
 * @param[in]  dev_sess    Session to DDF device providing USB HID interface.
 * @param[out] usage_pages Where to store usage pages.
 * @param[out] usages      Where to store usages (actual data).
 * @param[in]  usage_count Length of @p usage_pages and @p usages buffer
 *                         (in items, not bytes).
 * @param[out] actual_usage_count Number of usages actually returned by the
 *                                device driver.
 * @param[in] flags        Flags (see USBHID_IFACE_FLAG_*).
 *
 * @return Error code.
 *
 */
errno_t usbhid_dev_get_event(async_sess_t *dev_sess, uint8_t *buf,
    size_t size, size_t *actual_size, int *event_nr, unsigned int flags)
{
	if (!dev_sess)
		return EINVAL;
	
	if (buf == NULL)
		return ENOMEM;
	
	if (size == 0)
		return EINVAL;
	
	size_t buffer_size =  size;
	uint8_t *buffer = malloc(buffer_size);
	if (buffer == NULL)
		return ENOMEM;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	ipc_call_t opening_request_call;
	aid_t opening_request = async_send_2(exch,
	    DEV_IFACE_ID(USBHID_DEV_IFACE), IPC_M_USBHID_GET_EVENT,
	    flags, &opening_request_call);
	
	if (opening_request == 0) {
		async_exchange_end(exch);
		free(buffer);
		return ENOMEM;
	}
	
	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(exch, buffer, buffer_size,
	    &data_request_call);
	
	async_exchange_end(exch);
	
	if (data_request == 0) {
		async_forget(opening_request);
		free(buffer);
		return ENOMEM;
	}
	
	errno_t data_request_rc;
	errno_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);
	
	if (data_request_rc != EOK) {
		/* Prefer return code of the opening request. */
		if (opening_request_rc != EOK)
			return (errno_t) opening_request_rc;
		else
			return (errno_t) data_request_rc;
	}
	
	if (opening_request_rc != EOK)
		return (errno_t) opening_request_rc;
	
	size_t act_size = IPC_GET_ARG2(data_request_call);
	
	/* Copy the individual items. */
	memcpy(buf, buffer, act_size);
	
	if (actual_size != NULL)
		*actual_size = act_size;
	
	if (event_nr != NULL)
		*event_nr = IPC_GET_ARG1(opening_request_call);
	
	return EOK;
}

errno_t usbhid_dev_get_report_descriptor_length(async_sess_t *dev_sess,
    size_t *size)
{
	if (!dev_sess)
		return EINVAL;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	sysarg_t arg_size;
	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(USBHID_DEV_IFACE),
	    IPC_M_USBHID_GET_REPORT_DESCRIPTOR_LENGTH, &arg_size);
	
	async_exchange_end(exch);
	
	if (rc == EOK) {
		if (size != NULL)
			*size = (size_t) arg_size;
	}
	
	return rc;
}

errno_t usbhid_dev_get_report_descriptor(async_sess_t *dev_sess, uint8_t *buf,
    size_t size, size_t *actual_size)
{
	if (!dev_sess)
		return EINVAL;
	
	if (buf == NULL)
		return ENOMEM;
	
	if (size == 0)
		return EINVAL;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t opening_request = async_send_1(exch,
	    DEV_IFACE_ID(USBHID_DEV_IFACE), IPC_M_USBHID_GET_REPORT_DESCRIPTOR,
	    NULL);
	if (opening_request == 0) {
		async_exchange_end(exch);
		return ENOMEM;
	}
	
	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(exch, buf, size,
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
		/* Prefer return code of the opening request. */
		if (opening_request_rc != EOK)
			return (errno_t) opening_request_rc;
		else
			return (errno_t) data_request_rc;
	}
	
	if (opening_request_rc != EOK)
		return (errno_t) opening_request_rc;
	
	size_t act_size = IPC_GET_ARG2(data_request_call);
	
	if (actual_size != NULL)
		*actual_size = act_size;
	
	return EOK;
}

static void remote_usbhid_get_event_length(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhid_get_event(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhid_get_report_descriptor_length(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhid_get_report_descriptor(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
// static void remote_usbhid_(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB HID interface operations. */
static const remote_iface_func_ptr_t remote_usbhid_iface_ops [] = {
	[IPC_M_USBHID_GET_EVENT_LENGTH] = remote_usbhid_get_event_length,
	[IPC_M_USBHID_GET_EVENT] = remote_usbhid_get_event,
	[IPC_M_USBHID_GET_REPORT_DESCRIPTOR_LENGTH] =
	    remote_usbhid_get_report_descriptor_length,
	[IPC_M_USBHID_GET_REPORT_DESCRIPTOR] = remote_usbhid_get_report_descriptor
};

/** Remote USB HID interface structure.
 */
const remote_iface_t remote_usbhid_iface = {
	.method_count = ARRAY_SIZE(remote_usbhid_iface_ops),
	.methods = remote_usbhid_iface_ops
};

//usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;


void remote_usbhid_get_event_length(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	printf("remote_usbhid_get_event_length()\n");
	
	usbhid_iface_t *hid_iface = (usbhid_iface_t *) iface;

	if (!hid_iface->get_event_length) {
		printf("Get event length not set!\n");
		async_answer_0(callid, ENOTSUP);
		return;
	}

	size_t len = hid_iface->get_event_length(fun);
//	if (len == 0) {
//		len = EEMPTY;
//	}
	async_answer_1(callid, EOK, len);
	
//	if (len < 0) {
//		async_answer_0(callid, len);
//	} else {
//		async_answer_1(callid, EOK, len);
//	}
}

void remote_usbhid_get_event(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhid_iface_t *hid_iface = (usbhid_iface_t *) iface;

	if (!hid_iface->get_event) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	unsigned int flags = DEV_IPC_GET_ARG1(*call);

	size_t len;
	ipc_callid_t data_callid;
	if (!async_data_read_receive(&data_callid, &len)) {
		async_answer_0(callid, EPARTY);
		return;
	}
//	/* Check that length is even number. Truncate otherwise. */
//	if ((len % 2) == 1) {
//		len--;
//	}
	if (len == 0) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}

	errno_t rc;

	uint8_t *data = malloc(len);
	if (data == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		return;
	}

	size_t act_length;
	int event_nr;
	rc = hid_iface->get_event(fun, data, len, &act_length, &event_nr, flags);
	if (rc != EOK) {
		free(data);
		async_answer_0(data_callid, rc);
		async_answer_0(callid, rc);
		return;
	}
	if (act_length >= len) {
		/* This shall not happen. */
		// FIXME: how about an assert here?
		act_length = len;
	}

	async_data_read_finalize(data_callid, data, act_length);

	free(data);

	async_answer_1(callid, EOK, event_nr);
}

void remote_usbhid_get_report_descriptor_length(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhid_iface_t *hid_iface = (usbhid_iface_t *) iface;

	if (!hid_iface->get_report_descriptor_length) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	size_t len = hid_iface->get_report_descriptor_length(fun);
	async_answer_1(callid, EOK, (sysarg_t) len);
}

void remote_usbhid_get_report_descriptor(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhid_iface_t *hid_iface = (usbhid_iface_t *) iface;

	if (!hid_iface->get_report_descriptor) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	size_t len;
	ipc_callid_t data_callid;
	if (!async_data_read_receive(&data_callid, &len)) {
		async_answer_0(callid, EINVAL);
		return;
	}

	if (len == 0) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}

	uint8_t *descriptor = malloc(len);
	if (descriptor == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		return;
	}

	size_t act_len = 0;
	errno_t rc = hid_iface->get_report_descriptor(fun, descriptor, len,
	    &act_len);
	if (act_len > len) {
		rc = ELIMIT;
	}
	if (rc != EOK) {
		free(descriptor);
		async_answer_0(data_callid, rc);
		async_answer_0(callid, rc);
		return;
	}

	async_data_read_finalize(data_callid, descriptor, act_len);
	async_answer_0(callid, EOK);

	free(descriptor);
}



/**
 * @}
 */
