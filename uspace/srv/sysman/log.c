/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "log.h"

static FILE *log_file = NULL;
static log_level_t max_level = LVL_NOTE;

extern void sysman_log_init(log_level_t level)
{
	max_level = level;
}

void sysman_log(log_level_t level, const char *fmt, ...)
{
	if (level > max_level) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	
	vprintf(fmt, args);
	printf("\n");
	if (log_file != NULL) {
		vfprintf(log_file, fmt, args);
		fprintf(log_file, "\n");
		fflush(log_file);
	}

	va_end(args);
}

void sysman_log_tofile(void)
{
	assert(log_file == NULL);
	log_file = fopen("/root/sysman.log", "a");
	if (log_file == NULL) {
		sysman_log(LVL_ERROR, "Failed opening logfile: %s", str_error(errno));
	} else {
		sysman_log(LVL_NOTE, "--- Begin sysman log ---");
	}

	assert(log_file != NULL);
}
