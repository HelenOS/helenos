/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 *  @brief DDI.
 */

#include <ddi/ddi.h>
#include <proc/task.h>
#include <typedefs.h>

/** Enable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task   Task.
 * @param ioaddr Starting I/O space address.
 * @param size   Size of the enabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 */
errno_t ddi_iospace_enable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	return 0;
}

/** Disable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task   Task.
 * @param ioaddr Starting I/O space address.
 * @param size   Size of the disabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 */
errno_t ddi_iospace_disable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	return 0;
}

/** @}
 */
