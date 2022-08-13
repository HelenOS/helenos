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

#include <proc/task.h>
#include <stddef.h>
#include <adt/bitmap.h>
#include <stdlib.h>

/** Perform amd64 specific task initialization.
 *
 * @param task Task to be initialized.
 *
 */
void task_create_arch(task_t *task)
{
	task->arch.iomapver = 0;
	bitmap_initialize(&task->arch.iomap, 0, NULL);
}

/** Perform amd64 specific task destruction.
 *
 * @param task Task to be initialized.
 *
 */
void task_destroy_arch(task_t *task)
{
	if (task->arch.iomap.bits != NULL)
		free(task->arch.iomap.bits);
}

/** @}
 */
