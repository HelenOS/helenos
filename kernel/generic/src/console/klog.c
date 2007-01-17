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

/** @addtogroup genericklog
 * @{
 */
/** @file
 */

#include <mm/frame.h>
#include <sysinfo/sysinfo.h>
#include <console/klog.h>
#include <print.h>
#include <ddi/device.h>
#include <ddi/irq.h>
#include <ddi/ddi.h>
#include <ipc/irq.h>

/** Physical memory area used for klog. */
static parea_t klog_parea;
	
/*
 * For now, we use 0 as INR.
 * However, on some architectures 0 is the clock interrupt (e.g. amd64 and
 * ia32). It is therefore desirable to have architecture specific definition of
 * KLOG_VIRT_INR in the future.
 */
#define KLOG_VIRT_INR	0

/* Order of frame to be allocated for klog communication */
#define KLOG_ORDER	0

static char *klog;
static int klogsize;
static int klogpos;

SPINLOCK_INITIALIZE(klog_lock);

static irq_t klog_irq;

static irq_ownership_t klog_claim(void);

/** Initialize kernel logging facility
 *
 * Allocate pages that are to be shared with uspace for console data.
 * The shared area is a circular buffer. Userspace application may
 * be notified on new data with indication of position and size
 * of the data within the circular buffer.
 */
void klog_init(void)
{
	void *faddr;

	faddr = frame_alloc(KLOG_ORDER, FRAME_ATOMIC);
	if (!faddr)
		panic("Cannot allocate page for klog");
	klog = (char *) PA2KA(faddr);
	
	devno_t devno = device_assign_devno();
	
	klog_parea.pbase = (uintptr_t) faddr;
	klog_parea.vbase = (uintptr_t) klog;
	klog_parea.frames = 1 << KLOG_ORDER;
	klog_parea.cacheable = true;
	ddi_parea_register(&klog_parea);

	sysinfo_set_item_val("klog.faddr", NULL, (unative_t) faddr);
	sysinfo_set_item_val("klog.fcolor", NULL, (unative_t)
		PAGE_COLOR((uintptr_t) klog));
	sysinfo_set_item_val("klog.pages", NULL, 1 << KLOG_ORDER);
	sysinfo_set_item_val("klog.devno", NULL, devno);
	sysinfo_set_item_val("klog.inr", NULL, KLOG_VIRT_INR);

	irq_initialize(&klog_irq);
	klog_irq.devno = devno;
	klog_irq.inr = KLOG_VIRT_INR;
	klog_irq.claim = klog_claim;
	irq_register(&klog_irq);

	klogsize = PAGE_SIZE << KLOG_ORDER;
	klogpos = 0;
}

/** Allways refuse IRQ ownership.
 *
 * This is not a real IRQ, so we always decline.
 *
 * @return Always returns IRQ_DECLINE.
 */
irq_ownership_t klog_claim(void)
{
	return IRQ_DECLINE;
}

static void klog_vprintf(const char *fmt, va_list args)
{
	int ret;
	va_list atst;

	va_copy(atst, args);
	spinlock_lock(&klog_lock);

	ret = vsnprintf(klog+klogpos, klogsize-klogpos, fmt, atst);
	if (ret >= klogsize-klogpos) {
		klogpos = 0;
		if (ret >= klogsize)
			goto out;
	}
	ipc_irq_send_msg(&klog_irq, klogpos, ret, 0);
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

/** @}
 */
