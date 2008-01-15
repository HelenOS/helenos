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
#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <arch/types.h>
#include <arch.h>
#include <func.h>
#include <print.h>
#include <atomic.h>

#define BUFLEN 2048
static char debug_buffer[BUFLEN];
static size_t offset = 0;
/** Initialize stdout to something that does not print, but does not fail
 *
 * Save data in some buffer so that it could be retrieved in the debugger
 */
static void null_putchar(chardev_t *d, const char ch)
{
	if (offset >= BUFLEN)
		offset = 0;
	debug_buffer[offset++] = ch;
}

static chardev_operations_t null_stdout_ops = {
	.write = null_putchar
};

chardev_t null_stdout = {
	.name = "null",
	.op = &null_stdout_ops
};

/** Standard input character device. */
chardev_t *stdin = NULL;
chardev_t *stdout = &null_stdout;

/** Get character from character device. Do not echo character.
 *
 * @param chardev Character device.
 *
 * @return Character read.
 */
uint8_t _getc(chardev_t *chardev)
{
	uint8_t ch;
	ipl_t ipl;

	if (atomic_get(&haltstate)) {
		/* If we are here, we are hopefully on the processor, that 
		 * issued the 'halt' command, so proceed to read the character
		 * directly from input
		 */
		if (chardev->op->read)
			return chardev->op->read(chardev);
		/* no other way of interacting with user, halt */
		if (CPU)
			printf("cpu%d: ", CPU->id);
		else
			printf("cpu: ");
		printf("halted - no kconsole\n");
		cpu_halt();
	}

	waitq_sleep(&chardev->wq);
	ipl = interrupts_disable();
	spinlock_lock(&chardev->lock);
	ch = chardev->buffer[(chardev->index - chardev->counter) % CHARDEV_BUFLEN];
	chardev->counter--;
	spinlock_unlock(&chardev->lock);
	interrupts_restore(ipl);

	chardev->op->resume(chardev);

	return ch;
}

/** Get string from character device.
 *
 * Read characters from character device until first occurrence
 * of newline character.
 *
 * @param chardev Character device.
 * @param buf Buffer where to store string terminated by '\0'.
 * @param buflen Size of the buffer.
 *
 * @return Number of characters read.
 */
count_t gets(chardev_t *chardev, char *buf, size_t buflen)
{
	index_t index = 0;
	char ch;

	while (index < buflen) {
		ch = _getc(chardev);
		if (ch == '\b') {
			if (index > 0) {
				index--;
				/* Space backspace, space */
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			continue;
		} 
		putchar(ch);

		if (ch == '\n') { /* end of string => write 0, return */
			buf[index] = '\0';
			return (count_t) index;
		}
		buf[index++] = ch;
	}
	return (count_t) index;
}

/** Get character from device & echo it to screen */
uint8_t getc(chardev_t *chardev)
{
	uint8_t ch;

	ch = _getc(chardev);
	putchar(ch);
	return ch;
}

void putchar(char c)
{
	if (stdout->op->write)
		stdout->op->write(stdout, c);
}

/** @}
 */
