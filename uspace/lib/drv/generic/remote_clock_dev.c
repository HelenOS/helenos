/*
 * SPDX-FileCopyrightText: 2012 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <async.h>
#include <errno.h>
#include <time.h>
#include <macros.h>
#include <device/clock_dev.h>

#include <ops/clock_dev.h>
#include <ddf/driver.h>

static void remote_clock_time_get(ddf_fun_t *, void *, ipc_call_t *);
static void remote_clock_time_set(ddf_fun_t *, void *, ipc_call_t *);

/** Remote clock interface operations */
static const remote_iface_func_ptr_t remote_clock_dev_iface_ops[] = {
	[CLOCK_DEV_TIME_GET] = remote_clock_time_get,
	[CLOCK_DEV_TIME_SET] = remote_clock_time_set,
};

/** Remote clock interface structure
 *
 * Interface for processing requests from remote clients
 * addressed by the clock interface.
 */
const remote_iface_t remote_clock_dev_iface = {
	.method_count = ARRAY_SIZE(remote_clock_dev_iface_ops),
	.methods = remote_clock_dev_iface_ops,
};

/** Process the time_get() request from the remote client
 *
 * @param fun The function from which the time is read
 * @param ops The local ops structure
 *
 */
static void remote_clock_time_get(ddf_fun_t *fun, void *ops, ipc_call_t *call)
{
	clock_dev_ops_t *clock_dev_ops = (clock_dev_ops_t *) ops;
	struct tm t;
	errno_t rc;

	ipc_call_t data;
	size_t len;
	if (!async_data_read_receive(&data, &len)) {
		/* TODO: Handle protocol error */
		async_answer_0(call, EINVAL);
		return;
	}

	if (!clock_dev_ops->time_get) {
		/* The driver does not provide the time_get() functionality */
		async_answer_0(&data, ENOTSUP);
		async_answer_0(call, ENOTSUP);
		return;
	}

	rc = (*clock_dev_ops->time_get)(fun, &t);

	if (rc != EOK) {
		/* Some error occurred */
		async_answer_0(&data, rc);
		async_answer_0(call, rc);
		return;
	}

	/* The operation was successful */
	async_data_read_finalize(&data, &t, sizeof(struct tm));
	async_answer_0(call, rc);
}

/** Process the time_set() request from the remote client
 *
 * @param fun The function to which the data are written
 * @param ops The local ops structure
 *
 */
static void remote_clock_time_set(ddf_fun_t *fun, void *ops, ipc_call_t *call)
{
	clock_dev_ops_t *clock_dev_ops = (clock_dev_ops_t *) ops;
	errno_t rc;
	struct tm t;

	ipc_call_t data;
	size_t len;

	if (!async_data_write_receive(&data, &len)) {
		/* TODO: Handle protocol error */
		async_answer_0(call, EINVAL);
		return;
	}

	if (!clock_dev_ops->time_set) {
		/* The driver does not support the time_set() functionality */
		async_answer_0(&data, ENOTSUP);
		async_answer_0(call, ENOTSUP);
		return;
	}

	async_data_write_finalize(&data, &t, sizeof(struct tm));

	rc = (*clock_dev_ops->time_set)(fun, &t);

	async_answer_0(call, rc);
}

/**
 * @}
 */
