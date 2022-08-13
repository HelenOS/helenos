/*
 * SPDX-FileCopyrightText: 2012 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <ipc/dev_iface.h>
#include <device/clock_dev.h>
#include <errno.h>
#include <async.h>
#include <time.h>

/** Read the current time from the device
 *
 * @param sess     Session of the device
 * @param t        The current time that will be read from the device
 *
 * @return         EOK on success or an error code
 */
errno_t
clock_dev_time_get(async_sess_t *sess, struct tm *t)
{
	aid_t req;
	errno_t ret;

	async_exch_t *exch = async_exchange_begin(sess);

	req = async_send_1(exch, DEV_IFACE_ID(CLOCK_DEV_IFACE),
	    CLOCK_DEV_TIME_GET, NULL);
	ret = async_data_read_start(exch, t, sizeof(*t));

	async_exchange_end(exch);

	errno_t rc;
	if (ret != EOK) {
		async_forget(req);
		return ret;
	}

	async_wait_for(req, &rc);
	return (errno_t)rc;
}

/** Set the current time
 *
 * @param sess   Session of the device
 * @param t      The current time that will be written to the device
 *
 * @return       EOK on success or an error code
 */
errno_t
clock_dev_time_set(async_sess_t *sess, struct tm *t)
{
	aid_t req;
	errno_t ret;

	async_exch_t *exch = async_exchange_begin(sess);

	req = async_send_1(exch, DEV_IFACE_ID(CLOCK_DEV_IFACE),
	    CLOCK_DEV_TIME_SET, NULL);
	ret = async_data_write_start(exch, t, sizeof(*t));

	async_exchange_end(exch);

	errno_t rc;
	if (ret != EOK) {
		async_forget(req);
		return ret;
	}

	async_wait_for(req, &rc);
	return (errno_t)rc;
}

/** @}
 */
