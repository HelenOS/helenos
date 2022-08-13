/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */

#ifndef DDF_LOG_H_
#define DDF_LOG_H_

#include <io/log.h>
#include <io/verify.h>

extern errno_t ddf_log_init(const char *);
extern void ddf_msg(log_level_t, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);

extern void ddf_dump_buffer(char *, size_t, const void *, size_t, size_t,
    size_t);

#define ddf_log_fatal(msg...) ddf_msg(LVL_FATAL, msg)
#define ddf_log_error(msg...) ddf_msg(LVL_ERROR, msg)
#define ddf_log_warning(msg...) ddf_msg(LVL_WARN, msg)
#define ddf_log_note(msg...) ddf_msg(LVL_NOTE, msg)
#define ddf_log_debug(msg...) ddf_msg(LVL_DEBUG, msg)
#define ddf_log_verbose(msg...) ddf_msg(LVL_DEBUG2, msg)

#endif

/** @}
 */
