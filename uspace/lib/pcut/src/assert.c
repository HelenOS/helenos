/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file
 * Formatting and processing of failed assertion messages.
 *
 * We are using static buffers to prevent any calls to malloc()/free()
 * by the testing framework.
 */

/*
 * We need _BSD_SOURCE because of vsnprintf() when compiling under C89.
 * In newer versions of features.h, _DEFAULT_SOURCE must be defined as well.
 */
#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "internal.h"

#pragma warning(push, 0)
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#pragma warning(pop)


/** Maximum length of failed-assert message. */
#define MAX_MESSAGE_LENGTH 256

/** How many assertion messages we need to keep in memory at once. */
#define MESSAGE_BUFFER_COUNT 2

/** Assertion message buffers. */
static char message_buffer[MESSAGE_BUFFER_COUNT][MAX_MESSAGE_LENGTH + 1];

/** Currently active assertion buffer. */
static int message_buffer_index = 0;

void pcut_failed_assertion_fmt(const char *filename, int line, const char *fmt, ...) {
	va_list args;
	char *current_buffer = message_buffer[message_buffer_index];
	size_t offset = 0;
	message_buffer_index = (message_buffer_index + 1) % MESSAGE_BUFFER_COUNT;

	pcut_snprintf(current_buffer, MAX_MESSAGE_LENGTH, "%s:%d: ", filename, line);
	offset = pcut_str_size(current_buffer);

	if (offset + 1 < MAX_MESSAGE_LENGTH) {
		va_start(args, fmt);
		vsnprintf(current_buffer + offset, MAX_MESSAGE_LENGTH - offset, fmt, args);
		va_end(args);
	}

	pcut_failed_assertion(current_buffer);
}
