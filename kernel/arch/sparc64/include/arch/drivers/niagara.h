/*
 * SPDX-FileCopyrightText: 2008 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_NIAGARA_H
#define KERN_sparc64_NIAGARA_H

#include <proc/thread.h>
#include <console/chardev.h>

typedef struct {
	thread_t *thread;
	indev_t *srlnin;
} niagara_instance_t;

char niagara_getc(void);
void niagara_grab(void);
void niagara_release(void);
niagara_instance_t *niagarain_init(void);

#endif

/** @}
 */
