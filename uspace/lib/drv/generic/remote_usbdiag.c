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
	IPC_M_USBDIAG_TEST_IN,
	IPC_M_USBDIAG_TEST_OUT,
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

errno_t usbdiag_test_in(async_exch_t *exch,
    const usbdiag_test_params_t *params, usbdiag_test_results_t *results)
{
	if (!exch)
		return EBADMEM;

	ipc_call_t answer;
	aid_t req = async_send_1(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE),
	    IPC_M_USBDIAG_TEST_IN, &answer);

	errno_t rc = async_data_write_start(exch, params,
	    sizeof(usbdiag_test_params_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_read_start(exch, results,
	    sizeof(usbdiag_test_results_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);

	errno_t retval;
	async_wait_for(req, &retval);

	return (errno_t) retval;
}

errno_t usbdiag_test_out(async_exch_t *exch,
    const usbdiag_test_params_t *params, usbdiag_test_results_t *results)
{
	if (!exch)
		return EBADMEM;

	ipc_call_t answer;
	aid_t req = async_send_1(exch, DEV_IFACE_ID(USBDIAG_DEV_IFACE),
	    IPC_M_USBDIAG_TEST_OUT, &answer);

	errno_t rc = async_data_write_start(exch, params,
	    sizeof(usbdiag_test_params_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_read_start(exch, results,
	    sizeof(usbdiag_test_results_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);

	errno_t retval;
	async_wait_for(req, &retval);

	return (errno_t) retval;
}

static void remote_usbdiag_test_in(ddf_fun_t *, void *,
    cap_call_handle_t, ipc_call_t *);
static void remote_usbdiag_test_out(ddf_fun_t *, void *,
    cap_call_handle_t, ipc_call_t *);

/** Remote USB diagnostic interface operations. */
static const remote_iface_func_ptr_t remote_usbdiag_iface_ops [] = {
	[IPC_M_USBDIAG_TEST_IN] = remote_usbdiag_test_in,
	[IPC_M_USBDIAG_TEST_OUT] = remote_usbdiag_test_out
};

/** Remote USB diagnostic interface structure. */
const remote_iface_t remote_usbdiag_iface = {
	.method_count = ARRAY_SIZE(remote_usbdiag_iface_ops),
	.methods = remote_usbdiag_iface_ops,
};

void remote_usbdiag_test_in(ddf_fun_t *fun, void *iface,
    cap_call_handle_t chandle, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	size_t size;
	cap_call_handle_t data_chandle;
	if (!async_data_write_receive(&data_chandle, &size)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_params_t)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	usbdiag_test_params_t params;
	if (async_data_write_finalize(data_chandle, &params, size) != EOK) {
		async_answer_0(chandle, EINVAL);
		return;
	}

	usbdiag_test_results_t results;
	const errno_t ret = !diag_iface->test_in ? ENOTSUP :
	    diag_iface->test_in(fun, &params, &results);

	if (ret != EOK) {
		async_answer_0(chandle, ret);
		return;
	}

	if (!async_data_read_receive(&data_chandle, &size)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_results_t)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	if (async_data_read_finalize(data_chandle, &results, size) != EOK) {
		async_answer_0(chandle, EINVAL);
		return;
	}

	async_answer_0(chandle, ret);
}

void remote_usbdiag_test_out(ddf_fun_t *fun, void *iface,
    cap_call_handle_t chandle, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	size_t size;
	cap_call_handle_t data_chandle;
	if (!async_data_write_receive(&data_chandle, &size)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_params_t)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	usbdiag_test_params_t params;
	if (async_data_write_finalize(data_chandle, &params, size) != EOK) {
		async_answer_0(chandle, EINVAL);
		return;
	}

	usbdiag_test_results_t results;
	const errno_t ret = !diag_iface->test_out ? ENOTSUP :
	    diag_iface->test_out(fun, &params, &results);

	if (ret != EOK) {
		async_answer_0(chandle, ret);
		return;
	}

	if (!async_data_read_receive(&data_chandle, &size)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_results_t)) {
		async_answer_0(data_chandle, EINVAL);
		async_answer_0(chandle, EINVAL);
		return;
	}

	if (async_data_read_finalize(data_chandle, &results, size) != EOK) {
		async_answer_0(chandle, EINVAL);
		return;
	}

	async_answer_0(chandle, ret);
}

/**
 * @}
 */
