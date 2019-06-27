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
