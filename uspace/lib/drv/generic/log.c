/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */

#include <io/log.h>
#include <stdarg.h>
#include <ddf/log.h>

/** Initialize the logging system.
 *
 * @param drv_name Driver name, will be printed as part of message
 *
 */
errno_t ddf_log_init(const char *drv_name)
{
	return log_init(drv_name);
}

/** Log a driver message.
 *
 * @param level Message verbosity level. Message is only printed
 *              if verbosity is less than or equal to current
 *              reporting level.
 * @param fmt   Format string (no trailing newline)
 *
 */
void ddf_msg(log_level_t level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_msgv(LOG_DEFAULT, level, fmt, args);
	va_end(args);
}

/** @}
 */
