/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_proc
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_TASK_H_
#define KERN_ia32_TASK_H_

#include <stddef.h>
#include <adt/bitmap.h>

typedef struct {
	/** I/O Permission bitmap Generation counter. */
	size_t iomapver;
	/** I/O Permission bitmap. */
	bitmap_t iomap;
} task_arch_t;

#endif

/** @}
 */
