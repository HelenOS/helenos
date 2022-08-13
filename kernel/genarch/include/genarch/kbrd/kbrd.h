/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 */

#ifndef KERN_KBD_H_
#define KERN_KBD_H_

#include <console/chardev.h>
#include <proc/thread.h>
#include <synch/spinlock.h>

typedef struct {
	thread_t *thread;

	indev_t *sink;
	indev_t raw;

	SPINLOCK_DECLARE(keylock);        /**< keylock protects keyflags and lockflags. */
	volatile unsigned int keyflags;   /**< Tracking of multiple keypresses. */
	volatile unsigned int lockflags;  /**< Tracking of multiple keys lockings. */
} kbrd_instance_t;

extern kbrd_instance_t *kbrd_init(void);
extern indev_t *kbrd_wire(kbrd_instance_t *, indev_t *);

#endif

/** @}
 */
