/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <synch/waitq.h>
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
	waitq_t wq;

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
