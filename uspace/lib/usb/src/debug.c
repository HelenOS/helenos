/*
 * Copyright (c) 2010-2011 Vojtech Horky
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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Debugging and logging support.
 */
#include <adt/list.h>
#include <fibril_synch.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ddf/log.h>
#include <usb/debug.h>

/** Level of logging messages. */
static usb_log_level_t log_level = USB_LOG_LEVEL_WARNING;

/** Prefix for logging messages. */
static const char *log_prefix = "usb";

/** Serialization mutex for logging functions. */
static FIBRIL_MUTEX_INITIALIZE(log_serializer);

/** File where to store the log. */
static FILE *log_stream = NULL;


/** Enable logging.
 *
 * @param level Maximal enabled level (including this one).
 * @param message_prefix Prefix for each printed message.
 */
void usb_log_enable(usb_log_level_t level, const char *message_prefix)
{
	log_prefix = message_prefix;
	log_level = level;
	if (log_stream == NULL) {
		char *fname;
		int rc = asprintf(&fname, "/log/%s", message_prefix);
		if (rc > 0) {
			log_stream = fopen(fname, "w");
			if (log_stream != NULL)
				setvbuf(log_stream, NULL, _IOFBF, BUFSIZ);
			
			free(fname);
		}
	}
}

/** Get log level name prefix.
 *
 * @param level Log level.
 * @return String prefix for the message.
 */
static const char *log_level_name(usb_log_level_t level)
{
	switch (level) {
		case USB_LOG_LEVEL_FATAL:
			return " FATAL";
		case USB_LOG_LEVEL_ERROR:
			return " ERROR";
		case USB_LOG_LEVEL_WARNING:
			return " WARN";
		case USB_LOG_LEVEL_INFO:
			return " info";
		default:
			return "";
	}
}

/** Print logging message.
 *
 * @param level Verbosity level of the message.
 * @param format Formatting directive.
 */
void usb_log_printf(usb_log_level_t level, const char *format, ...)
{
	FILE *screen_stream = NULL;
	switch (level) {
		case USB_LOG_LEVEL_FATAL:
		case USB_LOG_LEVEL_ERROR:
			screen_stream = stderr;
			break;
		default:
			screen_stream = stdout;
			break;
	}
	assert(screen_stream != NULL);

	va_list args;

	/*
	 * Serialize access to log files.
	 * Print to screen only messages with higher level than the one
	 * specified during logging initialization.
	 * Print also to file, to it print one more (lower) level as well.
	 */
	fibril_mutex_lock(&log_serializer);

	const char *level_name = log_level_name(level);

	if ((log_stream != NULL) && (level <= log_level + 1)) {
		va_start(args, format);

		fprintf(log_stream, "[%s]%s: ", log_prefix, level_name);
		vfprintf(log_stream, format, args);
		fflush(log_stream);

		va_end(args);
	}

	if (level <= log_level) {
		va_start(args, format);

		fprintf(screen_stream, "[%s]%s: ", log_prefix, level_name);
		vfprintf(screen_stream, format, args);
		fflush(screen_stream);

		va_end(args);
	}

	fibril_mutex_unlock(&log_serializer);
}


#define REMAINDER_STR_FMT " (%zu)..."
/* string + terminator + number width (enough for 4GB)*/
#define REMAINDER_STR_LEN (5 + 1 + 10)

/** How many bytes to group together. */
#define BUFFER_DUMP_GROUP_SIZE 4

/** Size of the string for buffer dumps. */
#define BUFFER_DUMP_LEN 240 /* Ought to be enough for everybody ;-). */

/** Fibril local storage for the dumped buffer. */
static fibril_local char buffer_dump[2][BUFFER_DUMP_LEN];
/** Fibril local storage for buffer switching. */
static fibril_local int buffer_dump_index = 0;

/** Dump buffer into string.
 *
 * The function dumps given buffer into hexadecimal format and stores it
 * in a static fibril local string.
 * That means that you do not have to deallocate the string (actually, you
 * can not do that) and you do not have to guard it against concurrent
 * calls to it.
 * The only limitation is that each second call rewrites the buffer again
 * (internally, two buffer are used in cyclic manner).
 * Thus, it is necessary to copy the buffer elsewhere (that includes printing
 * to screen or writing to file).
 * Since this function is expected to be used for debugging prints only,
 * that is not a big limitation.
 *
 * @warning You cannot use this function more than twice in the same printf
 * (see detailed explanation).
 *
 * @param buffer Buffer to be printed (can be NULL).
 * @param size Size of the buffer in bytes (can be zero).
 * @param dumped_size How many bytes to actually dump (zero means all).
 * @return Dumped buffer as a static (but fibril local) string.
 */
const char *usb_debug_str_buffer(const uint8_t *buffer, size_t size,
    size_t dumped_size)
{
	/*
	 * Remove previous string.
	 */
	bzero(buffer_dump[buffer_dump_index], BUFFER_DUMP_LEN);

	/* Do the actual dump. */
	ddf_dump_buffer(buffer_dump[buffer_dump_index], BUFFER_DUMP_LEN,
	    buffer, 1, size, dumped_size);

	/* Next time, use the other buffer. */
	buffer_dump_index = 1 - buffer_dump_index;

	/* Need to take the old one due to previous line. */
	return buffer_dump[1 - buffer_dump_index];
}


/**
 * @}
 */
