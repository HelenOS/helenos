/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup libusbdiag
 * @{
 */
/** @file
 * USB diagnostic device remote interface.
 */

#include <async.h>
#include <assert.h>
#include <macros.h>
#include <errno.h>
#include <devman.h>

#include <usb/diag/iface.h>
#include "ddf/driver.h"

typedef enum {
	IPC_M_USB_DIAG_TEST,
} usb_iface_funcs_t;

async_sess_t *usb_diag_connect(devman_handle_t handle)
{
	return devman_device_connect(handle, IPC_FLAG_BLOCKING);
}

void usb_diag_disconnect(async_sess_t *sess)
{
	if (sess)
		async_hangup(sess);
}

int usb_diag_test(async_exch_t *exch, int x, int *y)
{
	if (!exch)
		return EBADMEM;

	sysarg_t y_;
	const int ret = async_req_2_1(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE), IPC_M_USB_DIAG_TEST, x, &y_);

	if (y)
		*y = (int) y_;

	return ret;
}

static void remote_usb_diag_test(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB diagnostic interface operations. */
static const remote_iface_func_ptr_t remote_usb_diag_iface_ops [] = {
	[IPC_M_USB_DIAG_TEST] = remote_usb_diag_test,
};

/** Remote USB diagnostic interface structure. */
const remote_iface_t remote_usb_diag_iface = {
	.method_count = ARRAY_SIZE(remote_usb_diag_iface_ops),
	.methods = remote_usb_diag_iface_ops,
};

void remote_usb_diag_test(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const usb_diag_iface_t *diag_iface = (usb_diag_iface_t *) iface;

	if (diag_iface->test == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int x = DEV_IPC_GET_ARG1(*call);
	int y;
	const int ret = diag_iface->test(fun, x, &y);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_1(callid, EOK, y);
	}
}

/**
 * @}
 */
