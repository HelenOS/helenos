/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jiri Svoboda
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
#include <fibril_synch.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <async.h>
#include <io/log.h>
#include <ipc/logger.h>
#include <str.h>
#include <ns.h>

/** Id of the first log we create at logger. */
static sysarg_t default_log_id;

/** Log messages are printed under this name. */
static const char *log_prog_name;

/** Names of individual log levels. */
static const char *log_level_names[] = {
	"fatal",
	"error",
	"warn",
	"note",
	"debug",
	"debug2",
	NULL
};

/** IPC session with the logger service. */
static async_sess_t *logger_session;

/** Maximum length of a single log message (in bytes). */
#define MESSAGE_BUFFER_SIZE 4096

/** Send formatted message to the logger service.
 *
 * @param session Initialized IPC session with the logger.
 * @param log Log to use.
 * @param level Verbosity level of the message.
 * @param message The actual message.
 * @return Error code of the conversion or EOK on success.
 */
static errno_t logger_message(async_sess_t *session, log_t log, log_level_t level, char *message)
{
	async_exch_t *exchange = async_exchange_begin(session);
	if (exchange == NULL) {
		return ENOMEM;
	}
	if (log == LOG_DEFAULT)
		log = default_log_id;

	// FIXME: remove when all USB drivers use libc logging explicitly
	str_rtrim(message, '\n');

	aid_t reg_msg = async_send_2(exchange, LOGGER_WRITER_MESSAGE,
	    log, level, NULL);
	errno_t rc = async_data_write_start(exchange, message, str_size(message));
	errno_t reg_msg_rc;
	async_wait_for(reg_msg, &reg_msg_rc);

	async_exchange_end(exchange);

	/*
	 * Getting ENAK means no-one wants our message. That is not an
	 * error at all.
	 */
	if (rc == ENAK)
		rc = EOK;

	if (rc != EOK) {
		return rc;
	}

	return reg_msg_rc;
}

/** Get name of the log level.
 *
 * @param level The log level.
 * @return String name or "unknown".
 */
const char *log_level_str(log_level_t level)
{
	if (level >= LVL_LIMIT)
		return "unknown";
	else
		return log_level_names[level];
}

/** Convert log level name to the enum.
 *
 * @param[in] name Log level name or log level number.
 * @param[out] level_out Where to store the result (set to NULL to ignore).
 * @return Error code of the conversion or EOK on success.
 */
errno_t log_level_from_str(const char *name, log_level_t *level_out)
{
	log_level_t level = LVL_FATAL;

	while (log_level_names[level] != NULL) {
		if (str_cmp(name, log_level_names[level]) == 0) {
			if (level_out != NULL)
				*level_out = level;
			return EOK;
		}
		level++;
	}

	/* Maybe user specified number directly. */
	char *end_ptr;
	int level_int = strtol(name, &end_ptr, 0);
	if ((end_ptr == name) || (str_length(end_ptr) != 0))
		return EINVAL;
	if (level_int < 0)
		return ERANGE;
	if (level_int >= (int) LVL_LIMIT)
		return ERANGE;

	if (level_out != NULL)
		*level_out = (log_level_t) level_int;

	return EOK;
}

/** Initialize the logging system.
 *
 * @param prog_name Program name, will be printed as part of message
 */
errno_t log_init(const char *prog_name)
{
	log_prog_name = str_dup(prog_name);
	if (log_prog_name == NULL)
		return ENOMEM;

	errno_t rc;
	logger_session = service_connect_blocking(SERVICE_LOGGER,
	    INTERFACE_LOGGER_WRITER, 0, &rc);
	if (logger_session == NULL)
		return rc;

	default_log_id = log_create(prog_name, LOG_NO_PARENT);

	return EOK;
}

/** Create a new (sub-) log.
 *
 * This function always returns a valid log_t. In case of errors,
 * @c parent is returned and errors are silently ignored.
 *
 * @param name Log name under which message will be reported (appended to parents name).
 * @param parent Parent log.
 * @return Opaque identifier of the newly created log.
 */
log_t log_create(const char *name, log_t parent)
{
	async_exch_t *exchange = async_exchange_begin(logger_session);
	if (exchange == NULL)
		return parent;

	if (parent == LOG_DEFAULT)
		parent = default_log_id;

	ipc_call_t answer;
	aid_t reg_msg = async_send_1(exchange, LOGGER_WRITER_CREATE_LOG,
	    parent, &answer);
	errno_t rc = async_data_write_start(exchange, name, str_size(name));
	errno_t reg_msg_rc;
	async_wait_for(reg_msg, &reg_msg_rc);

	async_exchange_end(exchange);

	if ((rc != EOK) || (reg_msg_rc != EOK))
		return parent;

	return ipc_get_arg1(&answer);
}

/** Write an entry to the log.
 *
 * The message is printed only if the verbosity level is less than or
 * equal to currently set reporting level of the log.
 *
 * @param ctx Log to use (use LOG_DEFAULT if you have no idea what it means).
 * @param level Severity level of the message.
 * @param fmt Format string in printf-like format (without trailing newline).
 */
void log_msg(log_t ctx, log_level_t level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_msgv(ctx, level, fmt, args);
	va_end(args);
}

/** Write an entry to the log (va_list variant).
 *
 * @param ctx Log to use (use LOG_DEFAULT if you have no idea what it means).
 * @param level Severity level of the message.
 * @param fmt Format string in printf-like format (without trailing newline).
 * @param args Arguments.
 */
void log_msgv(log_t ctx, log_level_t level, const char *fmt, va_list args)
{
	assert(level < LVL_LIMIT);

	char *message_buffer = malloc(MESSAGE_BUFFER_SIZE);
	if (message_buffer == NULL)
		return;

	vsnprintf(message_buffer, MESSAGE_BUFFER_SIZE, fmt, args);
	logger_message(logger_session, ctx, level, message_buffer);
	free(message_buffer);
}

/** @}
 */
