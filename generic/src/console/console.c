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

#include <console/console.h>
#include <console/chardev.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch.h>

/** Standard input character device. */
chardev_t *stdin = NULL;
chardev_t *stdout = NULL;

/** Get string from character device.
 *
 * Read characters from character device until first occurrence
 * of newline character.
 *
 * @param chardev Character device.
 * @param buf Buffer where to store string terminated by '\0'.
 * @param len Size of the buffer.
 *
 * @return Number of characters read.
 */
count_t gets(chardev_t *chardev, char *buf, size_t buflen)
{
	index_t index = 0;
	char ch;

	while (index < buflen) {
		ch = getc(chardev);
		if (ch == '\n') { /* end of string => write 0, return */
			buf[index] = '\0';
			return (count_t) index;
		}
		buf[index++] = ch;
	}
	return (count_t) index;
}

/** Get character from character device.
 *
 * @param chardev Character device.
 *
 * @return Character read.
 */
__u8 getc(chardev_t *chardev)
{
	__u8 ch;
	ipl_t ipl;

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

void putchar(char c)
{
	stdout->op->write(stdout, c);
}
