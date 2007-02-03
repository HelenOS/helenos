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

/** @addtogroup genericconsole
 * @{
 */
/** @file
 */

#ifndef KERN_CHARDEV_H_
#define KERN_CHARDEV_H_

#include <arch/types.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>

#define CHARDEV_BUFLEN 512

struct chardev;

/* Character device operations interface. */
typedef struct {
	/** Suspend pushing characters. */
	void (* suspend)(struct chardev *);
	/** Resume pushing characters. */
	void (* resume)(struct chardev *);
	/** Write character to stream. */
	void (* write)(struct chardev *, char c);
	/** Read character directly from device, assume interrupts disabled. */
	char (* read)(struct chardev *); 
} chardev_operations_t;

/** Character input device. */
typedef struct chardev {
	char *name;
	
	waitq_t wq;
	/** Protects everything below. */
	SPINLOCK_DECLARE(lock);	
	uint8_t buffer[CHARDEV_BUFLEN];
	count_t counter;
	/** Implementation of chardev operations. */
	chardev_operations_t *op;
	index_t index;
	void *data;
} chardev_t;

extern void chardev_initialize(char *name, chardev_t *chardev,
    chardev_operations_t *op);
extern void chardev_push_character(chardev_t *chardev, uint8_t ch);

#endif /* KERN_CHARDEV_H_ */

/** @}
 */
