/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kio
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
	char32_t *data;
} item_t;

static prodcons_t pc;

/* Pointer to kio area */
static char32_t *kio = (char32_t *) AS_AREA_ANY;
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
static void producer(size_t length, char32_t *data)
{
	item_t *item = (item_t *) malloc(sizeof(item_t));
	if (item == NULL)
		return;

	size_t sz = sizeof(char32_t) * length;
	char32_t *buf = (char32_t *) malloc(sz);
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
			putuchar(item->data[i]);

		if (log != NULL) {
			for (size_t i = 0; i < item->length; i++)
				fputuc(item->data[i], log);

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

	size_t kio_start = (size_t) ipc_get_arg1(call);
	size_t kio_len = (size_t) ipc_get_arg2(call);
	size_t kio_stored = (size_t) ipc_get_arg3(call);

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
	kio_length = size / sizeof(char32_t);

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
