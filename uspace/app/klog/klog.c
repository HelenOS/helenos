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
#include <ipc/ipc.h>
#include <async.h>
#include <ipc/services.h>
#include <as.h>
#include <sysinfo.h>
#include <event.h>
#include <errno.h>
#include <io/klog.h>

#define NAME  "klog"

/* Pointer to klog area */
static wchar_t *klog;
static size_t klog_length;

static void interrupt_received(ipc_callid_t callid, ipc_call_t *call)
{
	size_t klog_start = (size_t) IPC_GET_ARG1(*call);
	size_t klog_len = (size_t) IPC_GET_ARG2(*call);
	size_t klog_stored = (size_t) IPC_GET_ARG3(*call);
	size_t i;
	
	for (i = klog_len - klog_stored; i < klog_len; i++)
		putchar(klog[(klog_start + i) % klog_length]);
}

int main(int argc, char *argv[])
{
	size_t klog_pages = sysinfo_value("klog.pages");
	size_t klog_size = klog_pages * PAGE_SIZE;
	klog_length = klog_size / sizeof(wchar_t);
	
	klog = (wchar_t *) as_get_mappable_page(klog_size);
	if (klog == NULL) {
		printf(NAME ": Error allocating memory area\n");
		return -1;
	}
	
	int res = ipc_share_in_start_1_0(PHONE_NS, (void *) klog,
	    klog_size, SERVICE_MEM_KLOG);
	if (res != EOK) {
		printf(NAME ": Error initializing memory area\n");
		return -1;
	}
	
	if (event_subscribe(EVENT_KLOG, 0) != EOK) {
		printf(NAME ": Error registering klog notifications\n");
		return -1;
	}
	
	async_set_interrupt_received(interrupt_received);
	klog_update();
	async_manager();
	
	return 0;
}

/** @}
 */
