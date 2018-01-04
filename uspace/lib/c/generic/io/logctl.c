/*
 * Copyright (c) 2012 Vojtech Horky
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
		logger_session = service_connect_blocking(SERVICE_LOGGER,
		    INTERFACE_LOGGER_CONTROL, 0);
		if (logger_session == NULL)
			return ENOMEM;
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
