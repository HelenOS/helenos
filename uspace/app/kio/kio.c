/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2025 Jiří Zárevúcky
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

/** @addtogroup kio
 * @{
 */
/**
 * @file
 */

#include <_bits/decls.h>
#include <libarch/config.h>
#include <stdio.h>
#include <async.h>
#include <as.h>
#include <ddi.h>
#include <errno.h>
#include <str_error.h>
#include <io/kio.h>
#include <sysinfo.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <adt/prodcons.h>
#include <tinput.h>
#include <uchar.h>
#include <vfs/vfs.h>

#define NAME       "kio"
#define LOG_FNAME  "/log/kio"

/* Producer/consumer buffers */
typedef struct {
	link_t link;
	size_t bytes;
	char *data;
} item_t;

static prodcons_t pc;

/* Notification mutex */
static FIBRIL_MUTEX_INITIALIZE(mtx);

#define READ_BUFFER_SIZE PAGE_SIZE

static size_t current_at;
static char read_buffer[READ_BUFFER_SIZE];

/** Klog producer
 *
 * Copies the contents of a character buffer to local
 * producer/consumer queue.
 *
 * @param length Number of characters to copy.
 * @param data   Pointer to the kernel kio buffer.
 *
 */
static void producer(size_t bytes, char *data)
{
	item_t *item = malloc(sizeof(item_t));
	if (item == NULL)
		return;

	item->bytes = bytes;
	item->data = malloc(bytes);
	if (!item->data) {
		free(item);
		return;
	}

	memcpy(item->data, data, bytes);

	link_initialize(&item->link);
	prodcons_produce(&pc, &item->link);
}

/** Klog consumer
 *
 * Waits in an infinite loop for the character data created by
 * the producer and outputs them to stdout and optionally into
 * a file.
 *
 * @param data Unused.
 *
 * @return Always EOK (unreachable).
 *
 */
static errno_t consumer(void *data)
{
	FILE *log = fopen(LOG_FNAME, "a");
	if (log == NULL)
		printf("%s: Unable to create log file %s (%s)\n", NAME, LOG_FNAME,
		    str_error(errno));

	while (true) {
		link_t *link = prodcons_consume(&pc);
		item_t *item = list_get_instance(link, item_t, link);

		fwrite(item->data, 1, item->bytes, stdout);

		if (log) {
			fwrite(item->data, 1, item->bytes, log);
			fflush(log);
			vfs_sync(fileno(log));
		}

		free(item->data);
		free(item);
	}

	fclose(log);
	return EOK;
}

/** Kernel notification handler
 *
 * Receives kernel kio notifications.
 *
 * @param call   IPC call structure
 * @param arg    Local argument
 *
 */
static void kio_notification_handler(ipc_call_t *call, void *arg)
{
	size_t kio_written = (size_t) ipc_get_arg1(call);

	/*
	 * Make sure we process only a single notification
	 * at any time to limit the chance of the consumer
	 * starving.
	 */

	fibril_mutex_lock(&mtx);

	while (current_at != kio_written) {
		size_t read = kio_read(read_buffer, READ_BUFFER_SIZE, current_at);
		if (read == 0)
			break;

		current_at += read;

		if (read > READ_BUFFER_SIZE) {
			/* We missed some data. */
			// TODO: Send a message with the number of lost characters to the consumer.
			read = READ_BUFFER_SIZE;
		}

		producer(read, read_buffer);
	}

	async_event_unmask(EVENT_KIO);
	fibril_mutex_unlock(&mtx);
}

int main(int argc, char *argv[])
{
	prodcons_initialize(&pc);
	errno_t rc = async_event_subscribe(EVENT_KIO, kio_notification_handler, NULL);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to register kio notifications\n",
		    NAME);
		return rc;
	}

	fid_t fid = fibril_create(consumer, NULL);
	if (!fid) {
		fprintf(stderr, "%s: Unable to create consumer fibril\n",
		    NAME);
		return ENOMEM;
	}

	tinput_t *input = tinput_new();
	if (!input) {
		fprintf(stderr, "%s: Could not create input\n", NAME);
		return ENOMEM;
	}

	fibril_add_ready(fid);
	async_event_unmask(EVENT_KIO);
	kio_update();

	tinput_set_prompt(input, "kio> ");

	char *str;
	while ((rc = tinput_read(input, &str)) == EOK) {
		if (str_cmp(str, "") == 0) {
			free(str);
			continue;
		}

		kio_command(str, str_size(str));
		free(str);
	}

	if (rc == ENOENT)
		rc = EOK;

	return EOK;
}

/** @}
 */
