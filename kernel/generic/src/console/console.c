/*
 * Copyright (c) 2003 Josef Cejka
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup genericconsole
 * @{
 */
/** @file
 */

#include <console/console.h>
#include <console/chardev.h>
#include <sysinfo/sysinfo.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <arch/types.h>
#include <ddi/device.h>
#include <ddi/irq.h>
#include <ddi/ddi.h>
#include <ipc/irq.h>
#include <arch.h>
#include <func.h>
#include <print.h>
#include <atomic.h>

#define KLOG_SIZE PAGE_SIZE
#define KLOG_LATENCY 8

/**< Kernel log cyclic buffer */
static char klog[KLOG_SIZE] __attribute__ ((aligned (PAGE_SIZE)));

/**< Kernel log initialized */
static bool klog_inited = false;
/**< First kernel log characters */
static index_t klog_start = 0;
/**< Number of valid kernel log characters */
static size_t klog_len = 0;
/**< Number of stored (not printed) kernel log characters */
static size_t klog_stored = 0;
/**< Number of stored kernel log characters for uspace */
static size_t klog_uspace = 0;

/**< Silent output */
static bool silent = false;

/**< Kernel log spinlock */
SPINLOCK_INITIALIZE(klog_lock);

/** Physical memory area used for klog buffer */
static parea_t klog_parea;

/*
 * For now, we use 0 as INR.
 * However, it is therefore desirable to have architecture specific
 * definition of KLOG_VIRT_INR in the future.
 */
#define KLOG_VIRT_INR	0

static irq_t klog_irq;

static chardev_operations_t null_stdout_ops = {
	.suspend = NULL,
	.resume = NULL,
	.write = NULL,
	.read = NULL
};

chardev_t null_stdout = {
	.name = "null",
	.op = &null_stdout_ops
};

/** Allways refuse IRQ ownership.
 *
 * This is not a real IRQ, so we always decline.
 *
 * @return Always returns IRQ_DECLINE.
 */
static irq_ownership_t klog_claim(void *instance)
{
	return IRQ_DECLINE;
}

/** Standard input character device */
chardev_t *stdin = NULL;
chardev_t *stdout = &null_stdout;

/** Initialize kernel logging facility
 *
 * The shared area contains kernel cyclic buffer. Userspace application may
 * be notified on new data with indication of position and size
 * of the data within the circular buffer.
 */
void klog_init(void)
{
	void *faddr = (void *) KA2PA(klog);
	
	ASSERT((uintptr_t) faddr % FRAME_SIZE == 0);
	ASSERT(KLOG_SIZE % FRAME_SIZE == 0);

	devno_t devno = device_assign_devno();
	
	klog_parea.pbase = (uintptr_t) faddr;
	klog_parea.vbase = (uintptr_t) klog;
	klog_parea.frames = SIZE2FRAMES(KLOG_SIZE);
	klog_parea.cacheable = true;
	ddi_parea_register(&klog_parea);

	sysinfo_set_item_val("klog.faddr", NULL, (unative_t) faddr);
	sysinfo_set_item_val("klog.pages", NULL, SIZE2FRAMES(KLOG_SIZE));
	sysinfo_set_item_val("klog.devno", NULL, devno);
	sysinfo_set_item_val("klog.inr", NULL, KLOG_VIRT_INR);

	irq_initialize(&klog_irq);
	klog_irq.devno = devno;
	klog_irq.inr = KLOG_VIRT_INR;
	klog_irq.claim = klog_claim;
	irq_register(&klog_irq);
	
	spinlock_lock(&klog_lock);
	klog_inited = true;
	spinlock_unlock(&klog_lock);
}

void grab_console(void)
{
	silent = false;
	arch_grab_console();
}

void release_console(void)
{
	silent = true;
	arch_release_console();
}

/** Get character from character device. Do not echo character.
 *
 * @param chardev Character device.
 *
 * @return Character read.
 */
uint8_t _getc(chardev_t *chardev)
{
	uint8_t ch;
	ipl_t ipl;

	if (atomic_get(&haltstate)) {
		/* If we are here, we are hopefully on the processor, that 
		 * issued the 'halt' command, so proceed to read the character
		 * directly from input
		 */
		if (chardev->op->read)
			return chardev->op->read(chardev);
		/* no other way of interacting with user, halt */
		if (CPU)
			printf("cpu%u: ", CPU->id);
		else
			printf("cpu: ");
		printf("halted (no kconsole)\n");
		cpu_halt();
	}

	waitq_sleep(&chardev->wq);
	ipl = interrupts_disable();
	spinlock_lock(&chardev->lock);
	ch = chardev->buffer[(chardev->index - chardev->counter) % CHARDEV_BUFLEN];
	chardev->counter--;
	spinlock_unlock(&chardev->lock);
	interrupts_restore(ipl);

	chardev->op->resume(chardev);

	return ch;
}

/** Get string from character device.
 *
 * Read characters from character device until first occurrence
 * of newline character.
 *
 * @param chardev Character device.
 * @param buf Buffer where to store string terminated by '\0'.
 * @param buflen Size of the buffer.
 *
 * @return Number of characters read.
 */
count_t gets(chardev_t *chardev, char *buf, size_t buflen)
{
	index_t index = 0;
	char ch;
	
	while (index < buflen) {
		ch = _getc(chardev);
		if (ch == '\b') {
			if (index > 0) {
				index--;
				/* Space backspace, space */
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			continue;
		} 
		putchar(ch);
		
		if (ch == '\n') { /* end of string => write 0, return */
			buf[index] = '\0';
			return (count_t) index;
		}
		buf[index++] = ch;
	}
	return (count_t) index;
}

/** Get character from device & echo it to screen */
uint8_t getc(chardev_t *chardev)
{
	uint8_t ch;

	ch = _getc(chardev);
	putchar(ch);
	return ch;
}

void klog_update(void)
{
	spinlock_lock(&klog_lock);
	
	if ((klog_inited) && (klog_irq.notif_cfg.notify) && (klog_uspace > 0)) {
		ipc_irq_send_msg_3(&klog_irq, klog_start, klog_len, klog_uspace);
		klog_uspace = 0;
	}
	
	spinlock_unlock(&klog_lock);
}

void putchar(char c)
{
	spinlock_lock(&klog_lock);
	
	if ((klog_stored > 0) && (stdout->op->write)) {
		/* Print charaters stored in kernel log */
		index_t i;
		for (i = klog_len - klog_stored; i < klog_len; i++)
			stdout->op->write(stdout, klog[(klog_start + i) % KLOG_SIZE], silent);
		klog_stored = 0;
	}
	
	/* Store character in the cyclic kernel log */
	klog[(klog_start + klog_len) % KLOG_SIZE] = c;
	if (klog_len < KLOG_SIZE)
		klog_len++;
	else
		klog_start = (klog_start + 1) % KLOG_SIZE;
	
	if (stdout->op->write)
		stdout->op->write(stdout, c, silent);
	else {
		/* The character is just in the kernel log */
		if (klog_stored < klog_len)
			klog_stored++;
	}
	
	/* The character is stored for uspace */
	if (klog_uspace < klog_len)
		klog_uspace++;
	
	/* Check notify uspace to update */
	bool update;
	if ((klog_uspace > KLOG_LATENCY) || (c == '\n'))
		update = true;
	else
		update = false;
	
	spinlock_unlock(&klog_lock);
	
	if (update)
		klog_update();
}

/** @}
 */
