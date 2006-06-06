/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#include <mm/frame.h>
#include <sysinfo/sysinfo.h>
#include <console/klog.h>
#include <print.h>
#include <ipc/irq.h>

/* Order of frame to be allocated for klog communication */
#define KLOG_ORDER 0

static char *klog;
static int klogsize;
static int klogpos;

SPINLOCK_INITIALIZE(klog_lock);

/** Initialize kernel loggin facility
 *
 * Allocate pages that are to be shared if uspace for console data
 */
void klog_init(void)
{
	void *faddr;

	faddr = (void *)PFN2ADDR(frame_alloc(KLOG_ORDER, FRAME_ATOMIC));
	if (!faddr)
		panic("Cannot allocate page for klog");
	klog = (char *)PA2KA(faddr);
	
	sysinfo_set_item_val("klog.faddr", NULL, (__native)faddr);
	sysinfo_set_item_val("klog.pages", NULL, 1 << KLOG_ORDER);

	klogsize = PAGE_SIZE << KLOG_ORDER;
	klogpos = 0;
}

static void klog_vprintf(const char *fmt, va_list args)
{
	int ret;
	va_list atst;

	va_copy(atst, args);
	spinlock_lock(&klog_lock);

	ret = vsnprintf(klog+klogpos, klogsize-klogpos, fmt, atst);
	// Workaround around bad return value from vsnprintf
	if (ret+klogpos < klogsize)
		ret = 100;
	if (ret == klogsize-klogpos) {
		klogpos = 0;
		ret = vsnprintf(klog+klogpos, klogsize-klogpos, fmt, args);
		ret = 100;
		if (ret == klogsize)
			goto out;
	}
	ipc_irq_send_msg(IPC_IRQ_KLOG, klogpos, ret);
	klogpos += ret;
	if (klogpos >= klogsize)
		klogpos = 0;
out:
	spinlock_unlock(&klog_lock);
	va_end(atst);
}

/** Printf a message to kernel-uspace log */
void klog_printf(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	
	klog_vprintf(fmt, args);

	va_end(args);
}
