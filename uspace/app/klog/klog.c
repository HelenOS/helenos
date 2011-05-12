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

/** @addtogroup klog KLog
 * @brief HelenOS KLog
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <async.h>
#include <as.h>
#include <ddi.h>
#include <event.h>
#include <errno.h>
#include <str_error.h>
#include <io/klog.h>
#include <sysinfo.h>

#define NAME       "klog"
#define LOG_FNAME  "/log/klog"

/* Pointer to klog area */
static wchar_t *klog;
static size_t klog_length;

static FILE *log;

static void interrupt_received(ipc_callid_t callid, ipc_call_t *call)
{
	size_t klog_start = (size_t) IPC_GET_ARG1(*call);
	size_t klog_len = (size_t) IPC_GET_ARG2(*call);
	size_t klog_stored = (size_t) IPC_GET_ARG3(*call);
	size_t i;
	
	for (i = klog_len - klog_stored; i < klog_len; i++) {
		wchar_t ch = klog[(klog_start + i) % klog_length];
		
		putchar(ch);
		
		if (log != NULL)
			fputc(ch, log);
	}
	
	if (log != NULL) {
		fflush(log);
		fsync(fileno(log));
	}
}

int main(int argc, char *argv[])
{
	size_t pages;
	int rc = sysinfo_get_value("klog.pages", &pages);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to get number of klog pages\n",
		    NAME);
		return rc;
	}
	
	uintptr_t faddr;
	rc = sysinfo_get_value("klog.faddr", &faddr);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to get klog physical address\n",
		    NAME);
		return rc;
	}
	
	size_t size = pages * PAGE_SIZE;
	klog_length = size / sizeof(wchar_t);
	
	klog = (wchar_t *) as_get_mappable_page(size);
	if (klog == NULL) {
		fprintf(stderr, "%s: Unable to allocate virtual memory area\n",
		    NAME);
		return ENOMEM;
	}
	
	rc = physmem_map((void *) faddr, (void *) klog, pages,
	    AS_AREA_READ | AS_AREA_CACHEABLE);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to map klog\n", NAME);
		return rc;
	}
	
	rc = event_subscribe(EVENT_KLOG, 0);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to register klog notifications\n",
		    NAME);
		return rc;
	}
	
	log = fopen(LOG_FNAME, "a");
	if (log == NULL)
		printf("%s: Unable to create log file %s (%s)\n", NAME, LOG_FNAME,
		    str_error(errno));
	
	async_set_interrupt_received(interrupt_received);
	klog_update();
	async_manager();
	
	return 0;
}

/** @}
 */
