/*
 * Copyright (C) 2003 Josef Cejka
 * Copyright (C) 2005 Jakub Jermar
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

#include <arch/drivers/keyboard.h>
#include <console/chardev.h>
#include <console/console.h>
#include <arch/cp0.h>
#include <putchar.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <typedefs.h>
#include <arch/drivers/arc.h>

static void keyboard_enable(void);
static void keyboard_disable(void);
static void arc_kb_disable(void);
static void arc_kb_enable(void);

static chardev_t kbrd;

static chardev_operations_t arc_ops = {
	.resume = arc_kb_enable,
	.suspend = arc_kb_disable
};

static chardev_operations_t msim_ops = {
	.resume = keyboard_enable,
	.suspend = keyboard_disable
};

static int arc_kb_enabled;

/** Initialize keyboard subsystem. */
void keyboard_init(void)
{
	if (arc_enabled()) {
		chardev_initialize(&kbrd, &arc_ops);
		arc_kb_enabled = 1;
	} else {
		cp0_unmask_int(KEYBOARD_IRQ);
		chardev_initialize(&kbrd, &msim_ops);
	}
	stdin = &kbrd;
}

/** Process keyboard interrupt. */
void keyboard(void)
{
	char ch;

	ch = *((char *) KEYBOARD_ADDRESS);
	chardev_push_character(&kbrd, ch);
}

/* Called from getc(). */
void keyboard_enable(void)
{
	cp0_unmask_int(KEYBOARD_IRQ);
}

/* Called from getc(). */
void keyboard_disable(void)
{
	cp0_mask_int(KEYBOARD_IRQ);
}

/*****************************/
/* Arc keyboard */

void keyboard_poll(void)
{
	int ch;

	if (!arc_enabled() || !arc_kb_enabled)
		return;
	while ((ch = arc_getchar()) != -1)
		chardev_push_character(&kbrd, ch);		
}

static void arc_kb_enable(void)
{
	arc_kb_enabled = 1;
}

/* Called from getc(). */
static void arc_kb_disable(void)
{
	arc_kb_enabled = 0;
}
