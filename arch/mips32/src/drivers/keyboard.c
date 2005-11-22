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

static chardev_t kbrd;

static void keyboard_enable(void);

/** Initialize keyboard subsystem. */
void keyboard_init(void)
{
	cp0_unmask_int(KEYBOARD_IRQ);
	chardev_initialize(&kbrd, keyboard_enable);
	stdin = &kbrd;
}

/** Process keyboard interrupt.
 *
 * This function is called directly from the interrupt handler.
 * It feeds the keyboard buffer with characters read. When the buffer
 * is full, it simply masks the keyboard interrupt signal.
 */
void keyboard(void)
{
	char ch;

	spinlock_lock(&kbrd.lock);
	kbrd.counter++;
	if (kbrd.counter == CHARDEV_BUFLEN - 1) {
	        /* buffer full => disable keyboard interrupt */
		cp0_mask_int(KEYBOARD_IRQ);
	}

	ch = *((char *) KEYBOARD_ADDRESS);
	putchar(ch);
	kbrd.buffer[kbrd.index++] = ch;
	kbrd.index = kbrd.index % CHARDEV_BUFLEN; /* index modulo size of buffer */
	waitq_wakeup(&kbrd.wq, WAKEUP_FIRST);
	spinlock_unlock(&kbrd.lock);
}

/* Called from getc(). */
void keyboard_enable(void)
{
	cp0_unmask_int(KEYBOARD_IRQ);
}
