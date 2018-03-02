/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup kio KIO
 * @brief HelenOS KIO
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
#include <io/kio.h>
#include <sysinfo.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <adt/prodcons.h>
#include <tinput.h>
#include <vfs/vfs.h>

#define NAME       "kio"
#define LOG_FNAME  "/log/kio"

/* Producer/consumer buffers */
typedef struct {
	link_t link;
	size_t length;
	wchar_t *data;
} item_t;

static prodcons_t pc;

/* Pointer to kio area */
static wchar_t *kio = (wchar_t *) AS_AREA_ANY;
static size_t kio_length;

/* Notification mutex */
static FIBRIL_MUTEX_INITIALIZE(mtx);

/** Klog producer
 *
 * Copies the contents of a character buffer to local
 * producer/consumer queue.
 *
 * @param length Number of characters to copy.
 * @param data   Pointer to the kernel kio buffer.
 *
 */
static void producer(size_t length, wchar_t *data)
{
	item_t *item = (item_t *) malloc(sizeof(item_t));
	if (item == NULL)
		return;

	size_t sz = sizeof(wchar_t) * length;
	wchar_t *buf = (wchar_t *) malloc(sz);
	if (buf == NULL) {
		free(item);
		return;
	}

	memcpy(buf, data, sz);

	link_initialize(&item->link);
	item->length = length;
	item->data = buf;
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

		for (size_t i = 0; i < item->length; i++)
			putchar(item->data[i]);

		if (log != NULL) {
			for (size_t i = 0; i < item->length; i++)
				fputc(item->data[i], log);

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
	/*
	 * Make sure we process only a single notification
	 * at any time to limit the chance of the consumer
	 * starving.
	 *
	 * Note: Usually the automatic masking of the kio
	 * notifications on the kernel side does the trick
	 * of limiting the chance of accidentally copying
	 * the same data multiple times. However, due to
	 * the non-blocking architecture of kio notifications,
	 * this possibility cannot be generally avoided.
	 */

	fibril_mutex_lock(&mtx);

	size_t kio_start = (size_t) IPC_GET_ARG1(*call);
	size_t kio_len = (size_t) IPC_GET_ARG2(*call);
	size_t kio_stored = (size_t) IPC_GET_ARG3(*call);

	size_t offset = (kio_start + kio_len - kio_stored) % kio_length;

	/* Copy data from the ring buffer */
	if (offset + kio_stored >= kio_length) {
		size_t split = kio_length - offset;

		producer(split, kio + offset);
		producer(kio_stored - split, kio);
	} else
		producer(kio_stored, kio + offset);

	async_event_unmask(EVENT_KIO);
	fibril_mutex_unlock(&mtx);
}

int main(int argc, char *argv[])
{
	size_t pages;
	errno_t rc = sysinfo_get_value("kio.pages", &pages);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to get number of kio pages\n",
		    NAME);
		return rc;
	}

	uintptr_t faddr;
	rc = sysinfo_get_value("kio.faddr", &faddr);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to get kio physical address\n",
		    NAME);
		return rc;
	}

	size_t size = pages * PAGE_SIZE;
	kio_length = size / sizeof(wchar_t);

	rc = physmem_map(faddr, pages, AS_AREA_READ | AS_AREA_CACHEABLE,
	    (void *) &kio);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to map kio\n", NAME);
		return rc;
	}

	prodcons_initialize(&pc);
	rc = async_event_subscribe(EVENT_KIO, kio_notification_handler, NULL);
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
