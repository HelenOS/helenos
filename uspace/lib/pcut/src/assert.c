/*
 * Copyright (c) 2012-2013 Vojtech Horky
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

/**
 * @file
 * Formatting and processing of failed assertion messages.
 *
 * We are using static buffers to prevent any calls to malloc()/free()
 * by the testing framework.
 */

#include "internal.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>

/** Maximum length of failed-assert message. */
#define MAX_MESSAGE_LENGTH 256

/** How many assertion messages we need to keep in memory at once. */
#define MESSAGE_BUFFER_COUNT 2

/** Assertion message buffers. */
static char message_buffer[MESSAGE_BUFFER_COUNT][MAX_MESSAGE_LENGTH + 1];

/** Currently active assertion buffer. */
static int message_buffer_index = 0;

/** Announce that assertion failed.
 *
 * @warning This function may not return.
 *
 * @param fmt printf-style formatting string.
 *
 */
void pcut_failed_assertion_fmt(const char *fmt, ...) {
	char *current_buffer = message_buffer[message_buffer_index];
	message_buffer_index = (message_buffer_index + 1) % MESSAGE_BUFFER_COUNT;

	va_list args;
	va_start(args, fmt);
	vsnprintf(current_buffer, MAX_MESSAGE_LENGTH, fmt, args);
	va_end(args);

	pcut_failed_assertion(current_buffer);
}
