/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 * SPDX-FileCopyrightText: 2008 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_ddi
 * @{
 */
/** @file
 */

#include <ddi/ddi.h>
#include <proc/task.h>
#include <typedefs.h>
#include <stdlib.h>
#include <errno.h>

#define IO_MEMMAP_PAGES	16384
#define PORTS_PER_PAGE	4

/** Enable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task	 Task.
 * @param ioaddr Starting I/O space address.
 * @param size	 Size of the enabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 */
errno_t ddi_iospace_enable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	if (!task->arch.iomap) {
		task->arch.iomap = malloc(sizeof(bitmap_t));
		if (task->arch.iomap == NULL)
			return ENOMEM;

		void *store = malloc(bitmap_size(IO_MEMMAP_PAGES));
		if (store == NULL)
			return ENOMEM;

		bitmap_initialize(task->arch.iomap, IO_MEMMAP_PAGES, store);
		bitmap_clear_range(task->arch.iomap, 0, IO_MEMMAP_PAGES);
	}

	uintptr_t iopage = ioaddr / PORTS_PER_PAGE;
	size = ALIGN_UP(size + ioaddr - 4 * iopage, PORTS_PER_PAGE);
	bitmap_set_range(task->arch.iomap, iopage, size / 4);

	return EOK;
}

/** Disable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task	 Task.
 * @param ioaddr Starting I/O space address.
 * @param size	 Size of the disabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 */
errno_t ddi_iospace_disable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	if (!task->arch.iomap)
		return EINVAL;

	uintptr_t iopage = ioaddr / PORTS_PER_PAGE;
	size = ALIGN_UP(size + ioaddr - 4 * iopage, PORTS_PER_PAGE);
	bitmap_clear_range(task->arch.iomap, iopage, size / 4);

	return EOK;
}

/** @}
 */
