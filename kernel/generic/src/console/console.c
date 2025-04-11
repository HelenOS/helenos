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

/** @addtogroup kernel_generic_console
 * @{
 */
/** @file
 */

#include <abi/kio.h>
#include <arch.h>
#include <assert.h>
#include <atomic.h>
#include <console/chardev.h>
#include <console/console.h>
#include <ddi/ddi.h>
#include <ddi/irq.h>
#include <errno.h>
#include <ipc/event.h>
#include <ipc/irq.h>
#include <mm/frame.h> /* SIZE2FRAMES */
#include <panic.h>
#include <preemption.h>
#include <proc/thread.h>
#include <putchar.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>  /* malloc */
#include <str.h>
#include <synch/mutex.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <syscall/copy.h>
#include <sysinfo/sysinfo.h>
#include <typedefs.h>

#define KIO_PAGES    8
#define KIO_LENGTH   (KIO_PAGES * PAGE_SIZE / sizeof(char32_t))

/** Kernel log cyclic buffer */
char32_t kio[KIO_LENGTH] __attribute__((aligned(PAGE_SIZE)));

/** Kernel log initialized */
static atomic_bool kio_inited = ATOMIC_VAR_INIT(false);

/** A mutex for preventing interleaving of output lines from different threads.
 * May not be held in some circumstances, so locking of any internal shared
 * structures is still necessary.
 */
static MUTEX_INITIALIZE(console_mutex, MUTEX_RECURSIVE);

/** Number of characters written to buffer. Periodically overflows. */
static size_t kio_written = 0;

/** Number of characters written to output devices. Periodically overflows. */
static size_t kio_processed = 0;

/** Last notification sent to uspace. */
static size_t kio_notified = 0;

/** Kernel log spinlock */
IRQ_SPINLOCK_INITIALIZE(kio_lock);

/** Physical memory area used for kio buffer */
static parea_t kio_parea;

static indev_t stdin_sink;
static outdev_t stdout_source;

static void stdin_signal(indev_t *, indev_signal_t);

static indev_operations_t stdin_ops = {
	.poll = NULL,
	.signal = stdin_signal
};

static void stdout_write(outdev_t *, char32_t);
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

static void stdout_write(outdev_t *dev, char32_t ch)
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

	ddi_parea_init(&kio_parea);
	kio_parea.pbase = (uintptr_t) faddr;
	kio_parea.frames = SIZE2FRAMES(sizeof(kio));
	kio_parea.unpriv = false;
	kio_parea.mapped = false;
	ddi_parea_register(&kio_parea);

	sysinfo_set_item_val("kio.faddr", NULL, (sysarg_t) faddr);
	sysinfo_set_item_val("kio.pages", NULL, KIO_PAGES);

	event_set_unmask_callback(EVENT_KIO, kio_update);
	atomic_store(&kio_inited, true);
}

void grab_console(void)
{
	sysinfo_set_item_val("kconsole", NULL, true);
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
	sysinfo_set_item_val("kconsole", NULL, false);
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

void kio_update(void *event)
{
	if (!atomic_load(&kio_inited))
		return;

	irq_spinlock_lock(&kio_lock, true);

	if (kio_notified != kio_written) {
		if (event_notify_1(EVENT_KIO, true, kio_written) == EOK)
			kio_notified = kio_written;
	}

	irq_spinlock_unlock(&kio_lock, true);
}

/** Flush characters that are stored in the output buffer
 *
 */
void kio_flush(void)
{
	bool ordy = ((stdout) && (stdout->op->write));

	if (!ordy)
		return;

	irq_spinlock_lock(&kio_lock, true);

	/* Print characters that weren't printed earlier */
	while (kio_written != kio_processed) {
		char32_t tmp = kio[kio_processed % KIO_LENGTH];
		kio_processed++;

		/*
		 * We need to give up the spinlock for
		 * the physical operation of writing out
		 * the character.
		 */
		irq_spinlock_unlock(&kio_lock, true);
		stdout->op->write(stdout, tmp);
		irq_spinlock_lock(&kio_lock, true);
	}

	irq_spinlock_unlock(&kio_lock, true);
}

/** Put a character into the output buffer.
 *
 * The caller is required to hold kio_lock
 */
void kio_push_char(const char32_t ch)
{
	kio[kio_written % KIO_LENGTH] = ch;
	kio_written++;
}

void putuchar(const char32_t ch)
{
	bool ordy = ((stdout) && (stdout->op->write));

	irq_spinlock_lock(&kio_lock, true);
	kio_push_char(ch);
	irq_spinlock_unlock(&kio_lock, true);

	/* Output stored characters */
	kio_flush();

	if (!ordy) {
		/*
		 * No standard output routine defined yet.
		 * The character is still stored in the kernel log
		 * for possible future output.
		 *
		 * The early_putuchar() function is used to output
		 * the character for low-level debugging purposes.
		 * Note that the early_putuchar() function might be
		 * a no-op on certain hardware configurations.
		 */
		early_putuchar(ch);
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
sys_errno_t sys_kio(int cmd, uspace_addr_t buf, size_t size)
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
		data = (char *) malloc(size + 1);
		if (!data)
			return (sys_errno_t) ENOMEM;

		rc = copy_from_uspace(data, buf, size);
		if (rc) {
			free(data);
			return (sys_errno_t) rc;
		}
		data[size] = 0;

		uint8_t substitute = '\x1a';
		str_sanitize(data, size, substitute);

		switch (cmd) {
		case KIO_WRITE:
			printf("[%s(%lu)] %s\n", TASK->name,
			    (unsigned long) TASK->taskid, data);
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

/** Lock console output, ensuring that lines from different threads don't
 * interleave. Does nothing when preemption is disabled, so that debugging
 * and error printouts in sensitive areas still work.
 */
void console_lock(void)
{
	if (!PREEMPTION_DISABLED)
		mutex_lock(&console_mutex);
}

/** Unlocks console output. See console_lock()
 */
void console_unlock(void)
{
	if (!PREEMPTION_DISABLED)
		mutex_unlock(&console_mutex);
}

/** @}
 */
