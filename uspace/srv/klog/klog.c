/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2013 Martin Sucha
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

/** @addtogroup klog
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <async.h>
#include <as.h>
#include <ddi.h>
#include <errno.h>
#include <str_error.h>
#include <io/klog.h>
#include <sysinfo.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <adt/prodcons.h>
#include <io/log.h>
#include <io/logctl.h>

#define NAME  "klog"

typedef size_t __attribute__((aligned(1))) unaligned_size_t;

typedef struct {
	size_t entry_len;
	uint32_t serial;
	uint32_t facility;
	uint32_t level;
	char message[0];

} __attribute__((__packed__)) log_entry_t;

/* Producer/consumer buffers */
typedef struct {
	link_t link;
	size_t size;
	log_entry_t *data;
} item_t;

static prodcons_t pc;

/* Pointer to buffer where kernel stores new entries */
#define BUFFER_SIZE PAGE_SIZE
static void *buffer;

/* Notification mutex */
static FIBRIL_MUTEX_INITIALIZE(mtx);

static log_t kernel_ctx;
static const char *facility_name[] = {
	"other",
	"uspace",
	"arch"
};

#define facility_len (sizeof(facility_name) / sizeof(const char *))
static log_t facility_ctx[facility_len];

/** Klog producer
 *
 * Copies the log entries to a producer/consumer queue.
 *
 * @param length Number of characters to copy.
 * @param data   Pointer to the kernel klog buffer.
 *
 */
static void producer(void)
{
	size_t len = 0;
	errno_t rc = klog_read(buffer, BUFFER_SIZE, &len);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "klog_read failed, rc = %s",
		    str_error_name(rc));
		return;
	}

	size_t offset = 0;
	while (offset < len) {
		size_t entry_len = *((unaligned_size_t *) (buffer + offset));

		if (offset + entry_len > len || entry_len < sizeof(log_entry_t))
			break;

		log_entry_t *buf = malloc(entry_len + 1);
		if (buf == NULL)
			break;

		item_t *item = malloc(sizeof(item_t));
		if (item == NULL) {
			free(buf);
			break;
		}

		memcpy(buf, buffer + offset, entry_len);
		*((uint8_t *) buf + entry_len) = 0;
		link_initialize(&item->link);
		item->size = entry_len;
		item->data = buf;
		prodcons_produce(&pc, &item->link);

		offset += entry_len;
	}
}

/** Klog consumer
 *
 * Waits in an infinite loop for the log data created by
 * the producer and logs them to the logger.
 *
 * @param data Unused.
 *
 * @return Always EOK (unreachable).
 *
 */
static errno_t consumer(void *data)
{

	while (true) {
		link_t *link = prodcons_consume(&pc);
		item_t *item = list_get_instance(link, item_t, link);

		if (item->size < sizeof(log_entry_t)) {
			free(item->data);
			free(item);
			continue;
		}

		if (item->data->facility == LF_USPACE) {
			/* Avoid reposting messages */
			free(item->data);
			free(item);
			continue;
		}

		log_t ctx = kernel_ctx;
		if (item->data->facility < facility_len) {
			ctx = facility_ctx[item->data->facility];
		}

		log_level_t lvl = item->data->level;
		if (lvl > LVL_LIMIT)
			lvl = LVL_NOTE;

		log_msg(ctx, lvl, "%s", item->data->message);

		free(item->data);
		free(item);
	}

	return EOK;
}

/** Kernel notification handler
 *
 * Receives kernel klog notifications.
 *
 * @param call   IPC call structure
 * @param arg    Local argument
 *
 */
static void klog_notification_received(ipc_call_t *call, void *arg)
{
	/*
	 * Make sure we process only a single notification
	 * at any time to limit the chance of the consumer
	 * starving.
	 */

	fibril_mutex_lock(&mtx);

	producer();

	async_event_unmask(EVENT_KLOG);
	fibril_mutex_unlock(&mtx);
}

int main(int argc, char *argv[])
{
	errno_t rc = log_init(NAME);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to initialize log\n", NAME);
		return rc;
	}

	kernel_ctx = log_create("kernel", LOG_NO_PARENT);
	for (unsigned int i = 0; i < facility_len; i++) {
		facility_ctx[i] = log_create(facility_name[i], kernel_ctx);
	}

	buffer = malloc(BUFFER_SIZE);
	if (buffer == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to allocate buffer");
		return 1;
	}

	prodcons_initialize(&pc);
	rc = async_event_subscribe(EVENT_KLOG, klog_notification_received, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Unable to register klog notifications");
		return rc;
	}

	fid_t fid = fibril_create(consumer, NULL);
	if (!fid) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Unable to create consumer fibril");
		return ENOMEM;
	}

	fibril_add_ready(fid);
	async_event_unmask(EVENT_KLOG);

	fibril_mutex_lock(&mtx);
	producer();
	fibril_mutex_unlock(&mtx);

	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
