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
#include <ddi/irq.h>
#include <ddi/ddi.h>
#include <event/event.h>
#include <ipc/irq.h>
#include <arch.h>
#include <func.h>
#include <print.h>
#include <putchar.h>
#include <atomic.h>
#include <syscall/copy.h>
#include <errno.h>
#include <string.h>

#define KLOG_SIZE     PAGE_SIZE
#define KLOG_LENGTH   (KLOG_SIZE / sizeof(wchar_t))
#define KLOG_LATENCY  8

/** Kernel log cyclic buffer */
static wchar_t klog[KLOG_LENGTH] __attribute__ ((aligned (PAGE_SIZE)));

/** Kernel log initialized */
static bool klog_inited = false;
/** First kernel log characters */
static index_t klog_start = 0;
/** Number of valid kernel log characters */
static size_t klog_len = 0;
/** Number of stored (not printed) kernel log characters */
static size_t klog_stored = 0;
/** Number of stored kernel log characters for uspace */
static size_t klog_uspace = 0;

/** Silence output */
bool silent = false;

/** Kernel log spinlock */
SPINLOCK_INITIALIZE(klog_lock);

/** Physical memory area used for klog buffer */
static parea_t klog_parea;

/** Standard input and output character devices */
indev_t *stdin = NULL;
outdev_t *stdout = NULL;

/** Initialize kernel logging facility
 *
 * The shared area contains kernel cyclic buffer. Userspace application may
 * be notified on new data with indication of position and size
 * of the data within the circular buffer.
 *
 */
void klog_init(void)
{
	void *faddr = (void *) KA2PA(klog);
	
	ASSERT((uintptr_t) faddr % FRAME_SIZE == 0);
	ASSERT(KLOG_SIZE % FRAME_SIZE == 0);
	
	klog_parea.pbase = (uintptr_t) faddr;
	klog_parea.frames = SIZE2FRAMES(sizeof(klog));
	ddi_parea_register(&klog_parea);
	
	sysinfo_set_item_val("klog.faddr", NULL, (unative_t) faddr);
	sysinfo_set_item_val("klog.pages", NULL, SIZE2FRAMES(sizeof(klog)));
	
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

/** Tell kernel to get keyboard/console access again */
unative_t sys_debug_enable_console(void)
{
#ifdef CONFIG_KCONSOLE
	grab_console();
	return true;
#else
	return false;
#endif
}

/** Tell kernel to relinquish keyboard/console access */
unative_t sys_debug_disable_console(void)
{
	release_console();
	return true;
}

bool check_poll(indev_t *indev)
{
	if (indev == NULL)
		return false;
	
	if (indev->op == NULL)
		return false;
	
	return (indev->op->poll != NULL);
}

/** Get character from input character device. Do not echo character.
 *
 * @param indev Input character device.
 * @return Character read.
 *
 */
wchar_t _getc(indev_t *indev)
{
	if (atomic_get(&haltstate)) {
		/* If we are here, we are hopefully on the processor that
		 * issued the 'halt' command, so proceed to read the character
		 * directly from input
		 */
		if (check_poll(indev))
			return indev->op->poll(indev);
		
		/* No other way of interacting with user */
		interrupts_disable();
		
		if (CPU)
			printf("cpu%u: ", CPU->id);
		else
			printf("cpu: ");
		printf("halted (no polling input)\n");
		cpu_halt();
	}
	
	waitq_sleep(&indev->wq);
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&indev->lock);
	wchar_t ch = indev->buffer[(indev->index - indev->counter) % INDEV_BUFLEN];
	indev->counter--;
	spinlock_unlock(&indev->lock);
	interrupts_restore(ipl);
	
	return ch;
}

/** Get string from input character device.
 *
 * Read characters from input character device until first occurrence
 * of newline character.
 *
 * @param indev  Input character device.
 * @param buf    Buffer where to store string terminated by NULL.
 * @param buflen Size of the buffer.
 *
 * @return Number of characters read.
 *
 */
count_t gets(indev_t *indev, char *buf, size_t buflen)
{
	size_t offset = 0;
	count_t count = 0;
	buf[offset] = 0;
	
	wchar_t ch;
	while ((ch = _getc(indev)) != '\n') {
		if (ch == '\b') {
			if (count > 0) {
				/* Space, backspace, space */
				putchar('\b');
				putchar(' ');
				putchar('\b');
				
				count--;
				offset = str_lsize(buf, count);
				buf[offset] = 0;
			}
		}
		if (chr_encode(ch, buf, &offset, buflen - 1) == EOK) {
			putchar(ch);
			count++;
			buf[offset] = 0;
		}
	}
	
	return count;
}

/** Get character from input device & echo it to screen */
wchar_t getc(indev_t *indev)
{
	wchar_t ch = _getc(indev);
	putchar(ch);
	return ch;
}

void klog_update(void)
{
	spinlock_lock(&klog_lock);
	
	if ((klog_inited) && (event_is_subscribed(EVENT_KLOG)) && (klog_uspace > 0)) {
		event_notify_3(EVENT_KLOG, klog_start, klog_len, klog_uspace);
		klog_uspace = 0;
	}
	
	spinlock_unlock(&klog_lock);
}

void putchar(const wchar_t ch)
{
	spinlock_lock(&klog_lock);
	
	if ((klog_stored > 0) && (stdout) && (stdout->op->write)) {
		/* Print charaters stored in kernel log */
		index_t i;
		for (i = klog_len - klog_stored; i < klog_len; i++)
			stdout->op->write(stdout, klog[(klog_start + i) % KLOG_LENGTH], silent);
		klog_stored = 0;
	}
	
	/* Store character in the cyclic kernel log */
	klog[(klog_start + klog_len) % KLOG_LENGTH] = ch;
	if (klog_len < KLOG_LENGTH)
		klog_len++;
	else
		klog_start = (klog_start + 1) % KLOG_LENGTH;
	
	if ((stdout) && (stdout->op->write))
		stdout->op->write(stdout, ch, silent);
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
	if ((klog_uspace > KLOG_LATENCY) || (ch == '\n'))
		update = true;
	else
		update = false;
	
	spinlock_unlock(&klog_lock);
	
	if (update)
		klog_update();
}

/** Print using kernel facility
 *
 * Print to kernel log.
 *
 */
unative_t sys_klog(int fd, const void *buf, size_t size)
{
	char *data;
	int rc;
	
	if (size > PAGE_SIZE)
		return ELIMIT;
	
	if (size > 0) {
		data = (char *) malloc(size + 1, 0);
		if (!data)
			return ENOMEM;
		
		rc = copy_from_uspace(data, buf, size);
		if (rc) {
			free(data);
			return rc;
		}
		data[size] = 0;
		
		printf("%s", data);
		free(data);
	} else
		klog_update();
	
	return size;
}

/** @}
 */
