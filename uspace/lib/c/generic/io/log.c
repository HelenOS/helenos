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

#include <io/log.h>

/** Serialization mutex for logging functions. */
static FIBRIL_MUTEX_INITIALIZE(log_serializer);

/** Current log level. */
static log_level_t log_level;

static FILE *log_stream;

static const char *log_prog_name;

/** Prefixes for individual logging levels. */
static const char *log_level_names[] = {
	[LVL_FATAL] = "Fatal error",
	[LVL_ERROR] = "Error",
	[LVL_WARN] = "Warning",
	[LVL_NOTE] = "Note",
	[LVL_DEBUG] = "Debug",
	[LVL_DEBUG2] = "Debug2"
};

/** Initialize the logging system.
 *
 * @param prog_name	Program name, will be printed as part of message
 * @param level		Minimum message level to print
 */
int log_init(const char *prog_name, log_level_t level)
{
	assert(level < LVL_LIMIT);
	log_level = level;

	log_stream = stdout;
	log_prog_name = str_dup(prog_name);
	if (log_prog_name == NULL)
		return ENOMEM;

	return EOK;
}

/** Write an entry to the log.
 *
 * @param level		Message verbosity level. Message is only printed
 *			if verbosity is less than or equal to current
 *			reporting level.
 * @param fmt		Format string (no traling newline).
 */
void log_msg(log_level_t level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_msgv(level, fmt, args);
	va_end(args);
}

/** Write an entry to the log (va_list variant).
 *
 * @param level		Message verbosity level. Message is only printed
 *			if verbosity is less than or equal to current
 *			reporting level.
 * @param fmt		Format string (no trailing newline)
 */
void log_msgv(log_level_t level, const char *fmt, va_list args)
{
	assert(level < LVL_LIMIT);

	/* Higher number means higher verbosity. */
	if (level <= log_level) {
		fibril_mutex_lock(&log_serializer);

		fprintf(log_stream, "%s: %s: ", log_prog_name,
		    log_level_names[level]);
		vfprintf(log_stream, fmt, args);
		fputc('\n', log_stream);
		fflush(log_stream);

		fibril_mutex_unlock(&log_serializer);
	}
}

/** @}
 */
