/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_proc
 * @{
 */
/** @file
 */
#include <proc/task.h>

#ifndef KERN_ia64_TASK_H_
#define KERN_ia64_TASK_H_

#include <adt/bitmap.h>

typedef struct {
	bitmap_t *iomap;
} task_arch_t;

#define task_create_arch(t) { (t)->arch.iomap = NULL; }
#define task_destroy_arch(t)

#endif

/** @}
 */
