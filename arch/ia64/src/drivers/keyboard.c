/*
 * Copyright (C) 2005 Jakub Vana
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
#include <putchar.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <typedefs.h>
#include <print.h>

static void keyboard_enable(void);
static void keyboard_disable(void);

static chardev_t kbrd;
static chardev_operations_t ops = {
	.resume = keyboard_enable,
	.suspend = keyboard_disable
};

bool kb_disable;

/** Initialize keyboard subsystem. */
void keyboard_init(void)
{
	chardev_initialize(&kbrd, &ops);
	stdin = &kbrd;
	kb_disable = false;
}

/** Ask keyboard if a key was pressed. */
void poll_keyboard(void)
{
	char ch;

	if (kb_disable)
		return;

	ch = ski_getchar();
	if(ch == '\r')
		ch = '\n'; 
	if (ch)
		chardev_push_character(&kbrd, ch);
}

/* Called from getc(). */
void keyboard_enable(void)
{
	kb_disable = false;
}

/* Called from getc(). */
void keyboard_disable(void)
{
	kb_disable = true;	
}
