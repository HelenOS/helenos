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

#include "usbhid_iface.h"
#include "ddf/driver.h"

static void remote_usbhid_get_event_length(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhid_get_event(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhid_get_report_descriptor_length(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhid_get_report_descriptor(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
// static void remote_usbhid_(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB HID interface operations. */
static remote_iface_func_ptr_t remote_usbhid_iface_ops [] = {
	remote_usbhid_get_event_length,
	remote_usbhid_get_event,
	remote_usbhid_get_report_descriptor_length,
	remote_usbhid_get_report_descriptor
};

/** Remote USB HID interface structure.
 */
remote_iface_t remote_usbhid_iface = {
	.method_count = sizeof(remote_usbhid_iface_ops) /
	    sizeof(remote_usbhid_iface_ops[0]),
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

	int rc;

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
	int rc = hid_iface->get_report_descriptor(fun, descriptor, len,
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
