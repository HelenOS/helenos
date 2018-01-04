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

#include <assert.h>
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
#include <abi/kio.h>
#include <mm/frame.h> /* SIZE2FRAMES */
#include <mm/slab.h>  /* malloc */

#define KIO_PAGES    8
#define KIO_LENGTH   (KIO_PAGES * PAGE_SIZE / sizeof(wchar_t))

/** Kernel log cyclic buffer */
wchar_t kio[KIO_LENGTH] __attribute__((aligned(PAGE_SIZE)));

/** Kernel log initialized */
static atomic_t kio_inited = {false};

/** First kernel log characters */
static size_t kio_start = 0;

/** Number of valid kernel log characters */
static size_t kio_len = 0;

/** Number of stored (not printed) kernel log characters */
static size_t kio_stored = 0;

/** Number of stored kernel log characters for uspace */
static size_t kio_uspace = 0;

/** Kernel log spinlock */
SPINLOCK_INITIALIZE_NAME(kio_lock, "kio_lock");

/** Physical memory area used for kio buffer */
static parea_t kio_parea;

static indev_t stdin_sink;
static outdev_t stdout_source;

static void stdin_signal(indev_t *, indev_signal_t);

static indev_operations_t stdin_ops = {
	.poll = NULL,
	.signal = stdin_signal
};

static void stdout_write(outdev_t *, wchar_t);
static void stdout_redraw(outdev_t *);
static void stdout_scroll_up(outdev_t *);
static void stdout_scroll_down(outdev_t *);

static outdev_operations_t stdout_ops = {
	.write = stdout_write,
	.redraw = stdout_redraw,
	.scroll_up = stdout_scroll_up,
	.scroll_down = stdout_scroll_down
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

static void stdin_signal(indev_t *indev, indev_signal_t signal)
{
	switch (signal) {
	case INDEV_SIGNAL_SCROLL_UP:
		if (stdout != NULL)
			stdout_scroll_up(stdout);
		break;
	case INDEV_SIGNAL_SCROLL_DOWN:
		if (stdout != NULL)
			stdout_scroll_down(stdout);
		break;
	}
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
	list_foreach(dev->list, link, outdev_t, sink) {
		if ((sink) && (sink->op->write))
			sink->op->write(sink, ch);
	}
}

static void stdout_redraw(outdev_t *dev)
{
	list_foreach(dev->list, link, outdev_t, sink) {
		if ((sink) && (sink->op->redraw))
			sink->op->redraw(sink);
	}
}

static void stdout_scroll_up(outdev_t *dev)
{
	list_foreach(dev->list, link, outdev_t, sink) {
		if ((sink) && (sink->op->scroll_up))
			sink->op->scroll_up(sink);
	}
}

static void stdout_scroll_down(outdev_t *dev)
{
	list_foreach(dev->list, link, outdev_t, sink) {
		if ((sink) && (sink->op->scroll_down))
			sink->op->scroll_down(sink);
	}
}

/** Initialize kernel logging facility
 *
 * The shared area contains kernel cyclic buffer. Userspace application may
 * be notified on new data with indication of position and size
 * of the data within the circular buffer.
 *
 */
void kio_init(void)
{
	void *faddr = (void *) KA2PA(kio);
	
	assert((uintptr_t) faddr % FRAME_SIZE == 0);
	
	kio_parea.pbase = (uintptr_t) faddr;
	kio_parea.frames = SIZE2FRAMES(sizeof(kio));
	kio_parea.unpriv = false;
	kio_parea.mapped = false;
	ddi_parea_register(&kio_parea);
	
	sysinfo_set_item_val("kio.faddr", NULL, (sysarg_t) faddr);
	sysinfo_set_item_val("kio.pages", NULL, KIO_PAGES);
	
	event_set_unmask_callback(EVENT_KIO, kio_update);
	atomic_set(&kio_inited, true);
}

void grab_console(void)
{
	event_notify_1(EVENT_KCONSOLE, false, true);
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
	event_notify_1(EVENT_KCONSOLE, false, false);
}

/** Activate kernel console override */
sysarg_t sys_debug_console(void)
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

void kio_update(void *event)
{
	if (!atomic_get(&kio_inited))
		return;
	
	spinlock_lock(&kio_lock);
	
	if (kio_uspace > 0) {
		if (event_notify_3(EVENT_KIO, true, kio_start, kio_len,
		    kio_uspace) == EOK)
			kio_uspace = 0;
	}
	
	spinlock_unlock(&kio_lock);
}

/** Flush characters that are stored in the output buffer
 *
 */
void kio_flush(void)
{
	bool ordy = ((stdout) && (stdout->op->write));
	
	if (!ordy)
		return;

	spinlock_lock(&kio_lock);

	/* Print characters that weren't printed earlier */
	while (kio_stored > 0) {
		wchar_t tmp = kio[(kio_start + kio_len - kio_stored) % KIO_LENGTH];
		kio_stored--;

		/*
		 * We need to give up the spinlock for
		 * the physical operation of writing out
		 * the character.
		 */
		spinlock_unlock(&kio_lock);
		stdout->op->write(stdout, tmp);
		spinlock_lock(&kio_lock);
	}

	spinlock_unlock(&kio_lock);
}

/** Put a character into the output buffer.
 *
 * The caller is required to hold kio_lock
 */
void kio_push_char(const wchar_t ch)
{
	kio[(kio_start + kio_len) % KIO_LENGTH] = ch;
	if (kio_len < KIO_LENGTH)
		kio_len++;
	else
		kio_start = (kio_start + 1) % KIO_LENGTH;
	
	if (kio_stored < kio_len)
		kio_stored++;
	
	/* The character is stored for uspace */
	if (kio_uspace < kio_len)
		kio_uspace++;
}

void putchar(const wchar_t ch)
{
	bool ordy = ((stdout) && (stdout->op->write));
	
	spinlock_lock(&kio_lock);
	kio_push_char(ch);
	spinlock_unlock(&kio_lock);
	
	/* Output stored characters */
	kio_flush();
	
	if (!ordy) {
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
		kio_update(NULL);
}

/** Print using kernel facility
 *
 * Print to kernel log.
 *
 */
sys_errno_t sys_kio(int cmd, const void *buf, size_t size)
{
	char *data;
	errno_t rc;

	switch (cmd) {
	case KIO_UPDATE:
		kio_update(NULL);
		return EOK;
	case KIO_WRITE:
	case KIO_COMMAND:
		break;
	default:
		return ENOTSUP;
	}

	if (size > PAGE_SIZE)
		return (sys_errno_t) ELIMIT;
	
	if (size > 0) {
		data = (char *) malloc(size + 1, 0);
		if (!data)
			return (sys_errno_t) ENOMEM;
		
		rc = copy_from_uspace(data, buf, size);
		if (rc) {
			free(data);
			return (sys_errno_t) rc;
		}
		data[size] = 0;
		
		switch (cmd) {
		case KIO_WRITE:
			printf("%s", data);
			break;
		case KIO_COMMAND:
			if (!stdin)
				break;
			for (unsigned int i = 0; i < size; i++)
				indev_push_character(stdin, data[i]);
			indev_push_character(stdin, '\n');
			break;
		}

		free(data);
	}

	return EOK;
}

/** @}
 */
