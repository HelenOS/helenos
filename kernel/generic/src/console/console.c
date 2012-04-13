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
#include <typedefs.h>
#include <ddi/irq.h>
#include <ddi/ddi.h>
#include <ipc/event.h>
#include <ipc/irq.h>
#include <arch.h>
#include <panic.h>
#include <print.h>
#include <putchar.h>
#include <atomic.h>
#include <syscall/copy.h>
#include <errno.h>
#include <str.h>

#define KLOG_PAGES    8
#define KLOG_LENGTH   (KLOG_PAGES * PAGE_SIZE / sizeof(wchar_t))

/** Kernel log cyclic buffer */
wchar_t klog[KLOG_LENGTH] __attribute__((aligned(PAGE_SIZE)));

/** Kernel log initialized */
static atomic_t klog_inited = {false};

/** First kernel log characters */
static size_t klog_start = 0;

/** Number of valid kernel log characters */
static size_t klog_len = 0;

/** Number of stored (not printed) kernel log characters */
static size_t klog_stored = 0;

/** Number of stored kernel log characters for uspace */
static size_t klog_uspace = 0;

/** Kernel log spinlock */
SPINLOCK_STATIC_INITIALIZE_NAME(klog_lock, "klog_lock");

/** Physical memory area used for klog buffer */
static parea_t klog_parea;

static indev_t stdin_sink;
static outdev_t stdout_source;

static indev_operations_t stdin_ops = {
	.poll = NULL
};

static void stdout_write(outdev_t *, wchar_t);
static void stdout_redraw(outdev_t *);

static outdev_operations_t stdout_ops = {
	.write = stdout_write,
	.redraw = stdout_redraw
};

/** Override kernel console lockout */
bool console_override = false;

/** Standard input and output character devices */
indev_t *stdin = NULL;
outdev_t *stdout = NULL;

indev_t *stdin_wire(void)
{
	if (stdin == NULL) {
		indev_initialize("stdin", &stdin_sink, &stdin_ops);
		stdin = &stdin_sink;
	}
	
	return stdin;
}

void stdout_wire(outdev_t *outdev)
{
	if (stdout == NULL) {
		outdev_initialize("stdout", &stdout_source, &stdout_ops);
		stdout = &stdout_source;
	}
	
	list_append(&outdev->link, &stdout->list);
}

static void stdout_write(outdev_t *dev, wchar_t ch)
{
	list_foreach(dev->list, cur) {
		outdev_t *sink = list_get_instance(cur, outdev_t, link);
		if ((sink) && (sink->op->write))
			sink->op->write(sink, ch);
	}
}

static void stdout_redraw(outdev_t *dev)
{
	list_foreach(dev->list, cur) {
		outdev_t *sink = list_get_instance(cur, outdev_t, link);
		if ((sink) && (sink->op->redraw))
			sink->op->redraw(sink);
	}
}

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
	
	klog_parea.pbase = (uintptr_t) faddr;
	klog_parea.frames = SIZE2FRAMES(sizeof(klog));
	klog_parea.unpriv = false;
	klog_parea.mapped = false;
	ddi_parea_register(&klog_parea);
	
	sysinfo_set_item_val("klog.faddr", NULL, (sysarg_t) faddr);
	sysinfo_set_item_val("klog.pages", NULL, KLOG_PAGES);
	
	event_set_unmask_callback(EVENT_KLOG, klog_update);
	atomic_set(&klog_inited, true);
}

void grab_console(void)
{
	bool prev = console_override;
	
	console_override = true;
	if ((stdout) && (stdout->op->redraw))
		stdout->op->redraw(stdout);
	
	if ((stdin) && (!prev)) {
		/*
		 * Force the console to print the prompt.
		 */
		indev_push_character(stdin, '\n');
	}
}

void release_console(void)
{
	console_override = false;
}

/** Activate kernel console override */
sysarg_t sys_debug_activate_console(void)
{
#ifdef CONFIG_KCONSOLE
	grab_console();
	return true;
#else
	return false;
#endif
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
size_t gets(indev_t *indev, char *buf, size_t buflen)
{
	size_t offset = 0;
	size_t count = 0;
	buf[offset] = 0;
	
	wchar_t ch;
	while ((ch = indev_pop_character(indev)) != '\n') {
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
	wchar_t ch = indev_pop_character(indev);
	putchar(ch);
	return ch;
}

void klog_update(void *event)
{
	if (!atomic_get(&klog_inited))
		return;
	
	spinlock_lock(&klog_lock);
	
	if (klog_uspace > 0) {
		if (event_notify_3(EVENT_KLOG, true, klog_start, klog_len,
		    klog_uspace) == EOK)
			klog_uspace = 0;
	}
	
	spinlock_unlock(&klog_lock);
}

void putchar(const wchar_t ch)
{
	bool ordy = ((stdout) && (stdout->op->write));
	
	spinlock_lock(&klog_lock);
	
	/* Print charaters stored in kernel log */
	if (ordy) {
		while (klog_stored > 0) {
			wchar_t tmp = klog[(klog_start + klog_len - klog_stored) % KLOG_LENGTH];
			klog_stored--;
			
			/*
			 * We need to give up the spinlock for
			 * the physical operation of writting out
			 * the character.
			 */
			spinlock_unlock(&klog_lock);
			stdout->op->write(stdout, tmp);
			spinlock_lock(&klog_lock);
		}
	}
	
	/* Store character in the cyclic kernel log */
	klog[(klog_start + klog_len) % KLOG_LENGTH] = ch;
	if (klog_len < KLOG_LENGTH)
		klog_len++;
	else
		klog_start = (klog_start + 1) % KLOG_LENGTH;
	
	if (!ordy) {
		if (klog_stored < klog_len)
			klog_stored++;
	}
	
	/* The character is stored for uspace */
	if (klog_uspace < klog_len)
		klog_uspace++;
	
	spinlock_unlock(&klog_lock);
	
	if (ordy) {
		/*
		 * Output the character. In this case
		 * it should be no longer buffered.
		 */
		stdout->op->write(stdout, ch);
	} else {
		/*
		 * No standard output routine defined yet.
		 * The character is still stored in the kernel log
		 * for possible future output.
		 *
		 * The early_putchar() function is used to output
		 * the character for low-level debugging purposes.
		 * Note that the early_putc() function might be
		 * a no-op on certain hardware configurations.
		 */
		early_putchar(ch);
	}
	
	/* Force notification on newline */
	if (ch == '\n')
		klog_update(NULL);
}

/** Print using kernel facility
 *
 * Print to kernel log.
 *
 */
sysarg_t sys_klog(int fd, const void *buf, size_t size)
{
	char *data;
	int rc;
	
	if (size > PAGE_SIZE)
		return (sysarg_t) ELIMIT;
	
	if (size > 0) {
		data = (char *) malloc(size + 1, 0);
		if (!data)
			return (sysarg_t) ENOMEM;
		
		rc = copy_from_uspace(data, buf, size);
		if (rc) {
			free(data);
			return (sysarg_t) rc;
		}
		data[size] = 0;
		
		printf("%s", data);
		free(data);
	} else
		klog_update(NULL);
	
	return size;
}

/** @}
 */
