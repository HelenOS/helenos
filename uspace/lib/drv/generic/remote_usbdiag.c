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

/** @addtogroup libdrv
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

#include "usbdiag_iface.h"

typedef enum {
	IPC_M_USBDIAG_STRESS_INTR_IN,
	IPC_M_USBDIAG_STRESS_INTR_OUT,
	IPC_M_USBDIAG_STRESS_BULK_IN,
	IPC_M_USBDIAG_STRESS_BULK_OUT,
	IPC_M_USBDIAG_STRESS_ISOCH_IN,
	IPC_M_USBDIAG_STRESS_ISOCH_OUT
} usb_iface_funcs_t;

async_sess_t *usbdiag_connect(devman_handle_t handle)
{
	return devman_device_connect(handle, IPC_FLAG_BLOCKING);
}

void usbdiag_disconnect(async_sess_t *sess)
{
	if (sess)
		async_hangup(sess);
}

int usbdiag_stress_intr_in(async_exch_t *exch, int cycles, size_t size)
{
	if (!exch)
		return EBADMEM;

	return async_req_3_0(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE), IPC_M_USBDIAG_STRESS_INTR_IN, cycles, size);
}

int usbdiag_stress_intr_out(async_exch_t *exch, int cycles, size_t size)
{
	if (!exch)
		return EBADMEM;

	return async_req_3_0(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE), IPC_M_USBDIAG_STRESS_INTR_OUT, cycles, size);
}

int usbdiag_stress_bulk_in(async_exch_t *exch, int cycles, size_t size)
{
	if (!exch)
		return EBADMEM;

	return async_req_3_0(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE), IPC_M_USBDIAG_STRESS_BULK_IN, cycles, size);
}

int usbdiag_stress_bulk_out(async_exch_t *exch, int cycles, size_t size)
{
	if (!exch)
		return EBADMEM;

	return async_req_3_0(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE), IPC_M_USBDIAG_STRESS_BULK_OUT, cycles, size);
}

int usbdiag_stress_isoch_in(async_exch_t *exch, int cycles, size_t size)
{
	if (!exch)
		return EBADMEM;

	return async_req_3_0(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE), IPC_M_USBDIAG_STRESS_ISOCH_IN, cycles, size);
}

int usbdiag_stress_isoch_out(async_exch_t *exch, int cycles, size_t size)
{
	if (!exch)
		return EBADMEM;

	return async_req_3_0(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE), IPC_M_USBDIAG_STRESS_ISOCH_OUT, cycles, size);
}

static void remote_usbdiag_stress_intr_in(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbdiag_stress_intr_out(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbdiag_stress_bulk_in(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbdiag_stress_bulk_out(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbdiag_stress_isoch_in(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbdiag_stress_isoch_out(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB diagnostic interface operations. */
static const remote_iface_func_ptr_t remote_usbdiag_iface_ops [] = {
	[IPC_M_USBDIAG_STRESS_INTR_IN] = remote_usbdiag_stress_intr_in,
	[IPC_M_USBDIAG_STRESS_INTR_OUT] = remote_usbdiag_stress_intr_out,
	[IPC_M_USBDIAG_STRESS_BULK_IN] = remote_usbdiag_stress_bulk_in,
	[IPC_M_USBDIAG_STRESS_BULK_OUT] = remote_usbdiag_stress_bulk_out,
	[IPC_M_USBDIAG_STRESS_ISOCH_IN] = remote_usbdiag_stress_isoch_in,
	[IPC_M_USBDIAG_STRESS_ISOCH_OUT] = remote_usbdiag_stress_isoch_out
};

/** Remote USB diagnostic interface structure. */
const remote_iface_t remote_usbdiag_iface = {
	.method_count = ARRAY_SIZE(remote_usbdiag_iface_ops),
	.methods = remote_usbdiag_iface_ops,
};

void remote_usbdiag_stress_intr_in(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	if (diag_iface->stress_bulk_in == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int cycles = DEV_IPC_GET_ARG1(*call);
	size_t size = DEV_IPC_GET_ARG2(*call);
	const int ret = diag_iface->stress_intr_in(fun, cycles, size);
	async_answer_0(callid, ret);
}

void remote_usbdiag_stress_intr_out(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	if (diag_iface->stress_bulk_out == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int cycles = DEV_IPC_GET_ARG1(*call);
	size_t size = DEV_IPC_GET_ARG2(*call);
	const int ret = diag_iface->stress_intr_out(fun, cycles, size);
	async_answer_0(callid, ret);
}

void remote_usbdiag_stress_bulk_in(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	if (diag_iface->stress_bulk_in == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int cycles = DEV_IPC_GET_ARG1(*call);
	size_t size = DEV_IPC_GET_ARG2(*call);
	const int ret = diag_iface->stress_bulk_in(fun, cycles, size);
	async_answer_0(callid, ret);
}

void remote_usbdiag_stress_bulk_out(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	if (diag_iface->stress_bulk_out == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int cycles = DEV_IPC_GET_ARG1(*call);
	size_t size = DEV_IPC_GET_ARG2(*call);
	const int ret = diag_iface->stress_bulk_out(fun, cycles, size);
	async_answer_0(callid, ret);
}

void remote_usbdiag_stress_isoch_in(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	if (diag_iface->stress_isoch_in == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int cycles = DEV_IPC_GET_ARG1(*call);
	size_t size = DEV_IPC_GET_ARG2(*call);
	const int ret = diag_iface->stress_isoch_in(fun, cycles, size);
	async_answer_0(callid, ret);
}

void remote_usbdiag_stress_isoch_out(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	if (diag_iface->stress_isoch_out == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int cycles = DEV_IPC_GET_ARG1(*call);
	size_t size = DEV_IPC_GET_ARG2(*call);
	const int ret = diag_iface->stress_isoch_out(fun, cycles, size);
	async_answer_0(callid, ret);
}

/**
 * @}
 */
