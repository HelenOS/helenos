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

/** @addtogroup kernel_generic_console
 * @{
 */
/** @file
 */

#ifndef KERN_CHARDEV_H_
#define KERN_CHARDEV_H_

#include <adt/list.h>
#include <stdbool.h>
#include <stddef.h>
#include <synch/semaphore.h>
#include <synch/spinlock.h>

#define INDEV_BUFLEN  512

/** Input character device out-of-band signal type. */
typedef enum {
	INDEV_SIGNAL_SCROLL_UP = 0,
	INDEV_SIGNAL_SCROLL_DOWN
} indev_signal_t;

struct indev;

/** Input character device operations interface. */
typedef struct {
	/** Read character directly from device, assume interrupts disabled. */
	char32_t (*poll)(struct indev *);

	/** Signal out-of-band condition. */
	void (*signal)(struct indev *, indev_signal_t);
} indev_operations_t;

/** Character input device. */
typedef struct indev {
	const char *name;
	semaphore_t wq;

	/** Protects everything below. */
	IRQ_SPINLOCK_DECLARE(lock);
	char32_t buffer[INDEV_BUFLEN];
	size_t counter;

	/** Implementation of indev operations. */
	indev_operations_t *op;
	size_t index;
	void *data;
} indev_t;

struct outdev;

/** Output character device operations interface. */
typedef struct {
	/** Write character to output. */
	void (*write)(struct outdev *, char32_t);

	/** Redraw any previously cached characters. */
	void (*redraw)(struct outdev *);

	/** Scroll up in the device cache. */
	void (*scroll_up)(struct outdev *);

	/** Scroll down in the device cache. */
	void (*scroll_down)(struct outdev *);
} outdev_operations_t;

/** Character output device. */
typedef struct outdev {
	const char *name;

	/** Protects everything below. */
	SPINLOCK_DECLARE(lock);

	/** Fields suitable for multiplexing. */
	link_t link;
	list_t list;

	/** Implementation of outdev operations. */
	outdev_operations_t *op;
	void *data;
} outdev_t;

extern void indev_initialize(const char *, indev_t *,
    indev_operations_t *);
extern void indev_push_character(indev_t *, char32_t);
extern char32_t indev_pop_character(indev_t *);
extern void indev_signal(indev_t *, indev_signal_t);

extern void outdev_initialize(const char *, outdev_t *,
    outdev_operations_t *);

extern bool check_poll(indev_t *);

#endif /* KERN_CHARDEV_H_ */

/** @}
 */
