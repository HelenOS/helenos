/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

#ifndef _LIBC_IO_LOG_H_
#define _LIBC_IO_LOG_H_

#include <stdarg.h>
#include <inttypes.h>
#include <io/verify.h>
#include <types/common.h>

#include <abi/log.h>

/** Log itself (logging target). */
typedef sysarg_t log_t;
/** Formatting directive for printing log_t. */
#define PRIlogctx PRIxn

/** Default log (target). */
#define LOG_DEFAULT ((log_t) -1)

/** Use when creating new top-level log. */
#define LOG_NO_PARENT ((log_t) 0)

extern const char *log_level_str(log_level_t);
extern errno_t log_level_from_str(const char *, log_level_t *);

extern errno_t log_init(const char *);
extern log_t log_create(const char *, log_t);

extern void log_msg(log_t, log_level_t, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(3, 4);
extern void log_msgv(log_t, log_level_t, const char *, va_list);

#endif

/** @}
 */
