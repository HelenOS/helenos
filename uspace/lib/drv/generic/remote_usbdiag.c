/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void remote_usbdiag_test_in(ddf_fun_t *, void *, ipc_call_t *);
static void remote_usbdiag_test_out(ddf_fun_t *, void *, ipc_call_t *);

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

void remote_usbdiag_test_in(ddf_fun_t *fun, void *iface, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	ipc_call_t data;
	size_t size;
	if (!async_data_write_receive(&data, &size)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_params_t)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	usbdiag_test_params_t params;
	if (async_data_write_finalize(&data, &params, size) != EOK) {
		async_answer_0(call, EINVAL);
		return;
	}

	usbdiag_test_results_t results;
	const errno_t ret = !diag_iface->test_in ? ENOTSUP :
	    diag_iface->test_in(fun, &params, &results);

	if (ret != EOK) {
		async_answer_0(call, ret);
		return;
	}

	if (!async_data_read_receive(&data, &size)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_results_t)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (async_data_read_finalize(&data, &results, size) != EOK) {
		async_answer_0(call, EINVAL);
		return;
	}

	async_answer_0(call, ret);
}

void remote_usbdiag_test_out(ddf_fun_t *fun, void *iface, ipc_call_t *call)
{
	const usbdiag_iface_t *diag_iface = (usbdiag_iface_t *) iface;

	ipc_call_t data;
	size_t size;
	if (!async_data_write_receive(&data, &size)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_params_t)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	usbdiag_test_params_t params;
	if (async_data_write_finalize(&data, &params, size) != EOK) {
		async_answer_0(call, EINVAL);
		return;
	}

	usbdiag_test_results_t results;
	const errno_t ret = !diag_iface->test_out ? ENOTSUP :
	    diag_iface->test_out(fun, &params, &results);

	if (ret != EOK) {
		async_answer_0(call, ret);
		return;
	}

	if (!async_data_read_receive(&data, &size)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (size != sizeof(usbdiag_test_results_t)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (async_data_read_finalize(&data, &results, size) != EOK) {
		async_answer_0(call, EINVAL);
		return;
	}

	async_answer_0(call, ret);
}

/**
 * @}
 */
