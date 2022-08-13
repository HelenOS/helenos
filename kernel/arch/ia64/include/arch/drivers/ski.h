/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_SKI_H_
#define KERN_ia64_SKI_H_

#include <console/chardev.h>
#include <proc/thread.h>

typedef struct {
	thread_t *thread;
	indev_t *srlnin;
} ski_instance_t;

extern outdev_t *skiout_init(void);

extern ski_instance_t *skiin_init(void);
extern void skiin_wire(ski_instance_t *, indev_t *);

#endif

/** @}
 */
