/*
 * Copyright (c) 2003 Josef Cejka
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2025 Jiří Zárevúcky
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
#include <console/chardev.h>
#include <console/console.h>
#include <errno.h>
#include <ipc/event.h>
#include <log.h>
#include <panic.h>
#include <preemption.h>
#include <proc/task.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>  /* malloc */
#include <str.h>
#include <synch/mutex.h>
#include <synch/spinlock.h>
#include <syscall/copy.h>
#include <sysinfo/sysinfo.h>

#define KIO_PAGES    8
#define KIO_LENGTH   (KIO_PAGES * PAGE_SIZE)

/** Kernel log cyclic buffer */
static char kio[KIO_LENGTH];

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

static IRQ_SPINLOCK_INITIALIZE(flush_lock);

static IRQ_SPINLOCK_INITIALIZE(early_mbstate_lock);
static mbstate_t early_mbstate;

static indev_t stdin_sink;
static outdev_t stdout_source;

static void stdin_signal(indev_t *, indev_signal_t);

static indev_operations_t stdin_ops = {
	.poll = NULL,
	.signal = stdin_signal
};

static void stdout_write(outdev_t *, const char *, size_t);
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

static void stdout_write(outdev_t *dev, const char *s, size_t n)
{
	list_foreach(dev->list, link, outdev_t, sink) {
		if ((sink) && (sink->op->write))
			sink->op->write(sink, s, n);
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
 */
void kio_init(void)
{
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

	if (!irq_spinlock_trylock(&flush_lock)) {
		/* Someone is currently flushing. */
		irq_spinlock_unlock(&kio_lock, true);
		return;
	}

	/* A small-ish local buffer so that we can write to output in chunks. */
	char buffer[256];

	/* Print characters that weren't printed earlier */
	while (kio_written != kio_processed) {
		size_t offset = kio_processed % KIO_LENGTH;
		size_t len = min(kio_written - kio_processed, KIO_LENGTH - offset);
		len = min(len, sizeof(buffer));

		/* Take out a chunk of the big buffer. */
		memcpy(buffer, &kio[offset], len);
		kio_processed += len;

		/*
		 * We need to give up the spinlock for the physical operation of writing
		 * out the buffer.
		 */
		irq_spinlock_unlock(&kio_lock, true);
		stdout->op->write(stdout, buffer, len);
		irq_spinlock_lock(&kio_lock, true);
	}

	irq_spinlock_unlock(&flush_lock, false);
	irq_spinlock_unlock(&kio_lock, true);
}

void kio_push_bytes(const char *s, size_t n)
{
	/* Skip the section we know we can't keep. */
	if (n > KIO_LENGTH) {
		size_t lost = n - KIO_LENGTH;
		kio_written += lost;
		s += lost;
		n -= lost;
	}

	size_t offset = kio_written % KIO_LENGTH;
	if (offset + n > KIO_LENGTH) {
		size_t first = KIO_LENGTH - offset;
		size_t last = n - first;
		memcpy(kio + offset, s, first);
		memcpy(kio, s + first, last);
	} else {
		memcpy(kio + offset, s, n);
	}

	kio_written += n;
}

static void early_putstr(const char *s, size_t n)
{
	irq_spinlock_lock(&early_mbstate_lock, true);

	size_t offset = 0;
	char32_t c;

	while ((c = str_decode_r(s, &offset, n, U_SPECIAL, &early_mbstate)))
		early_putuchar(c);

	irq_spinlock_unlock(&early_mbstate_lock, true);
}

void putstr(const char *s, size_t n)
{
	bool ordy = ((stdout) && (stdout->op->write));

	irq_spinlock_lock(&kio_lock, true);
	kio_push_bytes(s, n);
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
		early_putstr(s, n);
	}

	/* Force notification when containing a newline */
	if (memchr(s, '\n', n) != NULL)
		kio_update(NULL);
}

/** Reads up to `size` characters from kio buffer starting at character `at`.
 *
 * @param size  Maximum number of characters that can be stored in buffer.
 *              Values greater than KIO_LENGTH are silently treated as KIO_LENGTH
 *              for the purposes of calculating the return value.
 * @return Number of characters read. Can be more than `size`.
 *         In that case, `size` characters are written to user buffer
 *         and the extra amount is the number of characters missed.
 */
sysarg_t sys_kio_read(uspace_addr_t buf, size_t size, size_t at)
{
	errno_t rc;
	size_t missed = 0;

	irq_spinlock_lock(&kio_lock, true);

	if (at == kio_written) {
		irq_spinlock_unlock(&kio_lock, true);
		return 0;
	}

	size_t readable_chars = kio_written - at;
	if (readable_chars > KIO_LENGTH) {
		missed = readable_chars - KIO_LENGTH;
		readable_chars = KIO_LENGTH;
	}

	size_t actual_read = min(readable_chars, size);
	size_t offset = (kio_written - readable_chars) % KIO_LENGTH;

	if (offset + actual_read > KIO_LENGTH) {
		size_t first = KIO_LENGTH - offset;
		size_t last = actual_read - first;

		rc = copy_to_uspace(buf, &kio[offset], first);
		if (rc == EOK)
			rc = copy_to_uspace(buf + first, &kio[0], last);
	} else {
		rc = copy_to_uspace(buf, &kio[offset], actual_read);
	}

	irq_spinlock_unlock(&kio_lock, true);

	if (rc != EOK) {
		log(LF_OTHER, LVL_WARN,
		    "[%s(%" PRIu64 ")] Terminating due to invalid memory buffer"
		    " in SYS_KIO_READ.\n", TASK->name, TASK->taskid);
		task_kill_self(true);
	}

	return actual_read + missed;
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
