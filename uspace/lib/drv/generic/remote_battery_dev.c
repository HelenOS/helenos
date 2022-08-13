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
#include <ops/battery_dev.h>
#include <battery_iface.h>
#include <ddf/driver.h>
#include <macros.h>

/** Read the current battery status from the device
 *
 * @param sess     Session of the device
 * @param status   Current status of the battery
 *
 * @return         EOK on success or an error code
 */
errno_t
battery_status_get(async_sess_t *sess, battery_status_t *batt_status)
{
	sysarg_t status;

	async_exch_t *exch = async_exchange_begin(sess);

	errno_t const rc = async_req_1_1(exch, DEV_IFACE_ID(BATTERY_DEV_IFACE),
	    BATTERY_STATUS_GET, &status);

	async_exchange_end(exch);

	if (rc == EOK)
		*batt_status = (battery_status_t) status;

	return rc;
}

/** Read the current battery charge level from the device
 *
 * @param sess     Session of the device
 * @param level    Battery charge level (0 - 100)
 *
 * @return         EOK on success or an error code
 */
errno_t
battery_charge_level_get(async_sess_t *sess, int *level)
{
	sysarg_t charge_level;

	async_exch_t *exch = async_exchange_begin(sess);

	errno_t const rc = async_req_1_1(exch, DEV_IFACE_ID(BATTERY_DEV_IFACE),
	    BATTERY_CHARGE_LEVEL_GET, &charge_level);

	async_exchange_end(exch);

	if (rc == EOK)
		*level = (int) charge_level;

	return rc;
}

static void remote_battery_status_get(ddf_fun_t *, void *, ipc_call_t *);
static void remote_battery_charge_level_get(ddf_fun_t *, void *, ipc_call_t *);

/** Remote battery interface operations */
static const remote_iface_func_ptr_t remote_battery_dev_iface_ops[] = {
	[BATTERY_STATUS_GET] = remote_battery_status_get,
	[BATTERY_CHARGE_LEVEL_GET] = remote_battery_charge_level_get,
};

/** Remote battery interface structure
 *
 * Interface for processing request from remote clients
 * addressed by the battery interface.
 *
 */
const remote_iface_t remote_battery_dev_iface = {
	.method_count = ARRAY_SIZE(remote_battery_dev_iface_ops),
	.methods = remote_battery_dev_iface_ops,
};

/** Process the status_get() request from the remote client
 *
 * @param fun The function from which the battery status is read
 * @param ops The local ops structure
 *
 */
static void remote_battery_status_get(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	const battery_dev_ops_t *bops = (battery_dev_ops_t *) ops;

	if (bops->battery_status_get == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	battery_status_t batt_status;
	const errno_t rc = bops->battery_status_get(fun, &batt_status);

	if (rc != EOK)
		async_answer_0(call, rc);
	else
		async_answer_1(call, rc, batt_status);
}

/** Process the battery_charge_level_get() request from the remote client
 *
 * @param fun  The function from which the battery charge level is read
 * @param ops The local ops structure
 *
 */
static void remote_battery_charge_level_get(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	const battery_dev_ops_t *bops = (battery_dev_ops_t *) ops;

	if (bops->battery_charge_level_get == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	int battery_level;
	const errno_t rc = bops->battery_charge_level_get(fun, &battery_level);

	if (rc != EOK)
		async_answer_0(call, rc);
	else
		async_answer_1(call, rc, battery_level);
}
