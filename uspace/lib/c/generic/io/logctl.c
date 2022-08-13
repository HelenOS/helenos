/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

#include <assert.h>
#include <errno.h>
#include <io/logctl.h>
#include <ipc/logger.h>
#include <sysinfo.h>
#include <ns.h>
#include <str.h>
#include <vfs/vfs.h>

/** IPC session with the logger service. */
static async_sess_t *logger_session = NULL;

static errno_t start_logger_exchange(async_exch_t **exchange_out)
{
	assert(exchange_out != NULL);

	if (logger_session == NULL) {
		errno_t rc;
		logger_session = service_connect_blocking(SERVICE_LOGGER,
		    INTERFACE_LOGGER_CONTROL, 0, &rc);
		if (logger_session == NULL)
			return rc;
	}

	assert(logger_session != NULL);

	async_exch_t *exchange = async_exchange_begin(logger_session);
	if (exchange == NULL)
		return ENOMEM;

	*exchange_out = exchange;

	return EOK;
}

/** Set default reported log level (global setting).
 *
 * This setting affects all logger clients whose reporting level was
 * not yet changed.
 *
 * If logging level of client A is changed with logctl_set_log_level()
 * to some level, this call will have no effect at that client's reporting
 * level. Even if the actual value of the reporting level of client A is
 * the same as current (previous) default log level.
 *
 * @param new_level New reported logging level.
 * @return Error code of the conversion or EOK on success.
 */
errno_t logctl_set_default_level(log_level_t new_level)
{
	async_exch_t *exchange = NULL;
	errno_t rc = start_logger_exchange(&exchange);
	if (rc != EOK)
		return rc;

	rc = (errno_t) async_req_1_0(exchange,
	    LOGGER_CONTROL_SET_DEFAULT_LEVEL, new_level);

	async_exchange_end(exchange);

	return rc;
}

/** Set reported log level of a single log.
 *
 * @see logctl_set_default_level
 *
 * @param logname Log name.
 * @param new_level New reported logging level.
 * @return Error code of the conversion or EOK on success.
 */
errno_t logctl_set_log_level(const char *logname, log_level_t new_level)
{
	async_exch_t *exchange = NULL;
	errno_t rc = start_logger_exchange(&exchange);
	if (rc != EOK)
		return rc;

	aid_t reg_msg = async_send_1(exchange, LOGGER_CONTROL_SET_LOG_LEVEL,
	    new_level, NULL);
	rc = async_data_write_start(exchange, logname, str_size(logname));
	errno_t reg_msg_rc;
	async_wait_for(reg_msg, &reg_msg_rc);

	async_exchange_end(exchange);

	if (rc != EOK)
		return rc;

	return (errno_t) reg_msg_rc;
}

/** Set logger's VFS root.
 *
 * @return Error code or EOK on success.
 */
errno_t logctl_set_root(void)
{
	async_exch_t *exchange = NULL;
	errno_t rc = start_logger_exchange(&exchange);
	if (rc != EOK)
		return rc;

	aid_t reg_msg = async_send_0(exchange, LOGGER_CONTROL_SET_ROOT, NULL);
	async_exch_t *vfs_exch = vfs_exchange_begin();
	rc = vfs_pass_handle(vfs_exch, vfs_root(), exchange);
	vfs_exchange_end(vfs_exch);
	errno_t reg_msg_rc;
	async_wait_for(reg_msg, &reg_msg_rc);

	async_exchange_end(exchange);

	if (rc != EOK)
		return rc;

	return (errno_t) reg_msg_rc;
}

/** @}
 */
