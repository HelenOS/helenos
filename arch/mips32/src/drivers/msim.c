/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#include <interrupt.h>
#include <console/chardev.h>
#include <arch/drivers/msim.h>
#include <arch/cp0.h>

static chardev_t console;

static void msim_write(chardev_t *dev, const char ch);
static void msim_enable(chardev_t *dev);
static void msim_disable(chardev_t *dev);

static chardev_operations_t msim_ops = {
	.resume = msim_enable,
	.suspend = msim_disable,
	.write = msim_write
};

/** Putchar that works with MSIM & gxemul */
void msim_write(chardev_t *dev, const char ch)
{
	*((char *) MSIM_VIDEORAM) = ch;
}

/* Called from getc(). */
void msim_enable(chardev_t *dev)
{
	cp0_unmask_int(MSIM_KBD_IRQ);
}

/* Called from getc(). */
void msim_disable(chardev_t *dev)
{
	cp0_mask_int(MSIM_KBD_IRQ);
}

/** Process keyboard interrupt. */
static void msim_interrupt(int n, void *stack)
{
	char ch;

	ch = *((char *) MSIM_KBD_ADDRESS);
	if (ch =='\r')
		ch = '\n';
	chardev_push_character(&console, ch);
}


/* Return console object representing msim console */
chardev_t * msim_console(void)
{
	chardev_initialize("msim_console", &console, &msim_ops);

	exc_register(MSIM_KBD_IRQ, "msim_kbd", msim_interrupt);

	cp0_unmask_int(MSIM_KBD_IRQ);

	return &console;
}
