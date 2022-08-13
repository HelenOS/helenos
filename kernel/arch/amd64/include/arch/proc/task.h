/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_proc
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_TASK_H_
#define KERN_amd64_TASK_H_

#include <adt/bitmap.h>
#include <stddef.h>

typedef struct {
	/** I/O Permission bitmap Generation counter. */
	size_t iomapver;
	/** I/O Permission bitmap. */
	bitmap_t iomap;
} task_arch_t;

#endif

/** @}
 */
