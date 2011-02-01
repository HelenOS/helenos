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
 * @brief Debugging support.
 */
#include <adt/list.h>
#include <fibril_synch.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <usb/debug.h>

/** Debugging tag. */
typedef struct {
	/** Linked list member. */
	link_t link;
	/** Tag name.
	 * We always have a private copy of the name.
	 */
	char *tag;
	/** Enabled level of debugging. */
	int level;
} usb_debug_tag_t;

/** Get instance of usb_debug_tag_t from link_t. */
#define USB_DEBUG_TAG_INSTANCE(iterator) \
	list_get_instance(iterator, usb_debug_tag_t, link)

/** List of all known tags. */
static LIST_INITIALIZE(tag_list);
/** Mutex guard for the list of all tags. */
static FIBRIL_MUTEX_INITIALIZE(tag_list_guard);

/** Level of logging messages. */
static usb_log_level_t log_level = USB_LOG_LEVEL_WARNING;
/** Prefix for logging messages. */
static const char *log_prefix = "usb";
/** Serialization mutex for logging functions. */
static FIBRIL_MUTEX_INITIALIZE(log_serializer);

/** Find or create new tag with given name.
 *
 * @param tagname Tag name.
 * @return Debug tag structure.
 * @retval NULL Out of memory.
 */
static usb_debug_tag_t *get_tag(const char *tagname)
{
	link_t *link;
	for (link = tag_list.next; \
	    link != &tag_list; \
	    link = link->next) {
		usb_debug_tag_t *tag = USB_DEBUG_TAG_INSTANCE(link);
		if (str_cmp(tag->tag, tagname) == 0) {
			return tag;
		}
	}

	/*
	 * Tag not found, we will create a new one.
	 */
	usb_debug_tag_t *new_tag = malloc(sizeof(usb_debug_tag_t));
	int rc = asprintf(&new_tag->tag, "%s", tagname);
	if (rc < 0) {
		free(new_tag);
		return NULL;
	}
	list_initialize(&new_tag->link);
	new_tag->level = 1;

	/*
	 * Append it to the end of known tags.
	 */
	list_append(&new_tag->link, &tag_list);

	return new_tag;
}

/** Print debugging information.
 * If the tag is used for the first time, its structures are automatically
 * created and initial verbosity level is set to 1.
 *
 * @param tagname Tag name.
 * @param level Level (verbosity) of the message.
 * @param format Formatting string for printf().
 */
void usb_dprintf(const char *tagname, int level, const char *format, ...)
{
	fibril_mutex_lock(&tag_list_guard);
	usb_debug_tag_t *tag = get_tag(tagname);
	if (tag == NULL) {
		printf("USB debug: FATAL ERROR - failed to create tag.\n");
		goto leave;
	}

	if (tag->level < level) {
		goto leave;
	}

	va_list args;
	va_start(args, format);

	printf("[%s:%d]: ", tagname, level);
	vprintf(format, args);

	va_end(args);

leave:
	fibril_mutex_unlock(&tag_list_guard);
}

/** Enable debugging prints for given tag.
 *
 * Setting level to <i>n</i> will cause that only printing messages
 * with level lower or equal to <i>n</i> will be printed.
 *
 * @param tagname Tag name.
 * @param level Enabled level.
 */
void usb_dprintf_enable(const char *tagname, int level)
{
	fibril_mutex_lock(&tag_list_guard);
	usb_debug_tag_t *tag = get_tag(tagname);
	if (tag == NULL) {
		printf("USB debug: FATAL ERROR - failed to create tag.\n");
		goto leave;
	}

	tag->level = level;

leave:
	fibril_mutex_unlock(&tag_list_guard);
}

/** Enable logging.
 *
 * @param level Maximal enabled level (including this one).
 * @param message_prefix Prefix for each printed message.
 */
void usb_log_enable(usb_log_level_t level, const char *message_prefix)
{
	log_prefix = message_prefix;
	log_level = level;
}


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
	if (level > log_level) {
		return;
	}

	FILE *stream = NULL;
	switch (level) {
		case USB_LOG_LEVEL_FATAL:
		case USB_LOG_LEVEL_ERROR:
			stream = stderr;
			break;
		default:
			stream = stdout;
			break;
	}
	assert(stream != NULL);

	va_list args;
	va_start(args, format);

	fibril_mutex_lock(&log_serializer);
	fprintf(stream, "[%s]%s: ", log_prefix, log_level_name(level));
	vfprintf(stream, format, args);
	fibril_mutex_unlock(&log_serializer);

	va_end(args);
}

/**
 * @}
 */
