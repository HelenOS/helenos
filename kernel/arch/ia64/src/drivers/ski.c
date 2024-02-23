/*
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

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#include <arch/drivers/ski.h>
#include <assert.h>
#include <console/console.h>
#include <console/chardev.h>
#include <ddi/ddi.h>
#include <sysinfo/sysinfo.h>
#include <stdint.h>
#include <proc/thread.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <arch/drivers/kbd.h>
#include <str.h>
#include <arch.h>
#include <stdlib.h>

enum {
	/** Interval between polling in microseconds */
	POLL_INTERVAL = 10000,  /* 0.01 s */

	/** Max. number of characters to pull out at a time */
	POLL_LIMIT = 30,

	SKI_INIT_CONSOLE = 20,
	SKI_GETCHAR      = 21,
	SKI_PUTCHAR      = 31
};

static void ski_putuchar(outdev_t *, const char32_t);

static outdev_operations_t skidev_ops = {
	.write = ski_putuchar,
	.redraw = NULL,
	.scroll_up = NULL,
	.scroll_down = NULL
};

static ski_instance_t *instance = NULL;
static parea_t ski_parea;

/** Ask debug console if a key was pressed.
 *
 * Use SSC (Simulator System Call) to
 * get character from debug console.
 *
 * This call is non-blocking.
 *
 * @return ASCII code of pressed key or 0 if no key pressed.
 *
 */
static char32_t ski_getchar(void)
{
	uint64_t ch;

	asm volatile (
	    "mov r15 = %1\n"
	    "break 0x80000;;\n"  /* modifies r8 */
	    "mov %0 = r8;;\n"

	    : "=r" (ch)
	    : "i" (SKI_GETCHAR)
	    : "r15", "r8"
	);

	return (char32_t) ch;
}

/** Ask keyboard if a key was pressed.
 *
 * If so, it will repeat and pull up to POLL_LIMIT characters.
 */
static void poll_keyboard(ski_instance_t *instance)
{
	int count = POLL_LIMIT;

	if (ski_parea.mapped && !console_override)
		return;

	while (count > 0) {
		char32_t ch = ski_getchar();

		if (ch == '\0')
			break;

		indev_push_character(instance->srlnin, ch);
		--count;
	}
}

/** Kernel thread for polling keyboard. */
static void kskipoll(void *arg)
{
	ski_instance_t *instance = (ski_instance_t *) arg;

	while (true) {
		poll_keyboard(instance);
		thread_usleep(POLL_INTERVAL);
	}
}

/** Initialize debug console
 *
 * Issue SSC (Simulator System Call) to
 * to open debug console.
 *
 */
static void ski_init(void)
{
	uintptr_t faddr;

	if (instance)
		return;

	asm volatile (
	    "mov r15 = %0\n"
	    "break 0x80000\n"
	    :
	    : "i" (SKI_INIT_CONSOLE)
	    : "r15", "r8"
	);

	faddr = frame_alloc(1, FRAME_LOWMEM | FRAME_ATOMIC, 0);
	if (faddr == 0)
		panic("Cannot allocate page for ski console.");

	ddi_parea_init(&ski_parea);
	ski_parea.pbase = faddr;
	ski_parea.frames = 1;
	ski_parea.unpriv = false;
	ski_parea.mapped = false;
	ddi_parea_register(&ski_parea);

	sysinfo_set_item_val("ski.paddr", NULL, (sysarg_t) faddr);

	instance = malloc(sizeof(ski_instance_t));

	if (instance) {
		instance->thread = thread_create(kskipoll, instance, TASK,
		    THREAD_FLAG_UNCOUNTED, "kskipoll");

		if (!instance->thread) {
			free(instance);
			instance = NULL;
			return;
		}

		instance->srlnin = NULL;
	}
}

static void ski_do_putchar(char ch)
{
	asm volatile (
	    "mov r15 = %[cmd]\n"
	    "mov r32 = %[ch]\n"   /* r32 is in0 */
	    "break 0x80000\n"     /* modifies r8 */
	    :
	    : [cmd] "i" (SKI_PUTCHAR), [ch] "r" (ch)
	    : "r15", "in0", "r8"
	);
}

/** Display character on debug console
 *
 * Use SSC (Simulator System Call) to
 * display character on debug console.
 *
 * @param dev    Character device.
 * @param ch     Character to be printed.
 *
 */
static void ski_putuchar(outdev_t *dev, char32_t ch)
{
	if (ski_parea.mapped && !console_override)
		return;

	if (ascii_check(ch)) {
		if (ch == '\n')
			ski_do_putchar('\r');

		ski_do_putchar(ch);
	} else {
		ski_do_putchar('?');
	}
}

outdev_t *skiout_init(void)
{
	ski_init();
	if (!instance)
		return NULL;

	outdev_t *skidev = malloc(sizeof(outdev_t));
	if (!skidev)
		return NULL;

	outdev_initialize("skidev", skidev, &skidev_ops);
	skidev->data = instance;

	if (!fb_exported) {
		/*
		 * This is the necessary evil until
		 * the userspace driver is entirely
		 * self-sufficient.
		 */
		sysinfo_set_item_val("fb", NULL, true);
		sysinfo_set_item_val("fb.kind", NULL, 6);

		fb_exported = true;
	}

	return skidev;
}

ski_instance_t *skiin_init(void)
{
	ski_init();
	return instance;
}

void skiin_wire(ski_instance_t *instance, indev_t *srlnin)
{
	assert(instance);
	assert(srlnin);

	instance->srlnin = srlnin;
	thread_start(instance->thread);

	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SKI);
}

/** @}
 */
