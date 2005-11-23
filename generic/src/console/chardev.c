/*
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

#include <console/chardev.h>
#include <putchar.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>

/** Initialize character device.
 *
 * @param chardev Character device.
 * @param op Implementation of character device operations.
 */
void chardev_initialize(chardev_t *chardev, chardev_operations_t *op)
{
	waitq_initialize(&chardev->wq);
	spinlock_initialize(&chardev->lock);
	chardev->counter = 0;
	chardev->index = 0;
	chardev->op = op;
}

/** Push character read from input character device.
 *
 * @param chardev Character device.
 * @param ch Character being pushed.
 */
void chardev_push_character(chardev_t *chardev, __u8 ch)
{
        spinlock_lock(&chardev->lock);
	chardev->counter++;
	if (chardev->counter == CHARDEV_BUFLEN - 1) {
		/* buffer full => disable device interrupt */
		chardev->op->suspend();
	}

	putchar(ch);
        chardev->buffer[chardev->index++] = ch;
        chardev->index = chardev->index % CHARDEV_BUFLEN; /* index modulo size of buffer */
        waitq_wakeup(&chardev->wq, WAKEUP_FIRST);
        spinlock_unlock(&chardev->lock);
}
