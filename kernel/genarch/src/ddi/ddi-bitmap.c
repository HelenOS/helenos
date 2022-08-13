/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_ddi
 * @{
 */
/** @file
 */

#include <ddi/ddi.h>
#include <arch/ddi/ddi.h>
#include <proc/task.h>
#include <typedefs.h>
#include <adt/bitmap.h>
#include <stdlib.h>
#include <arch/pm.h>
#include <errno.h>
#include <arch/cpu.h>
#include <cpu.h>
#include <arch.h>
#include <align.h>

/** Enable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task   Task.
 * @param ioaddr Starting I/O space address.
 * @param size   Size of the enabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 *
 */
errno_t ddi_iospace_enable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	size_t elements = ioaddr + size;
	if (elements > IO_PORTS)
		return ENOENT;

	if (task->arch.iomap.elements < elements) {
		/*
		 * The I/O permission bitmap is too small and needs to be grown.
		 */

		void *store = malloc(bitmap_size(elements));
		if (!store)
			return ENOMEM;

		bitmap_t oldiomap;
		bitmap_initialize(&oldiomap, task->arch.iomap.elements,
		    task->arch.iomap.bits);

		bitmap_initialize(&task->arch.iomap, elements, store);

		/*
		 * Mark the new range inaccessible.
		 */
		bitmap_set_range(&task->arch.iomap, oldiomap.elements,
		    elements - oldiomap.elements);

		/*
		 * In case there really existed smaller iomap,
		 * copy its contents and deallocate it.
		 */
		if (oldiomap.bits) {
			bitmap_copy(&task->arch.iomap, &oldiomap,
			    oldiomap.elements);

			free(oldiomap.bits);
		}
	}

	/*
	 * Enable the range and we are done.
	 */
	bitmap_clear_range(&task->arch.iomap, (size_t) ioaddr, size);

	/*
	 * Increment I/O Permission bitmap generation counter.
	 */
	task->arch.iomapver++;

	return EOK;
}

/** Disable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task   Task.
 * @param ioaddr Starting I/O space address.
 * @param size   Size of the enabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 *
 */
errno_t ddi_iospace_disable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	size_t elements = ioaddr + size;
	if (elements > IO_PORTS)
		return ENOENT;

	if (ioaddr >= task->arch.iomap.elements)
		return EINVAL;

	if (task->arch.iomap.elements < elements)
		size -= elements - task->arch.iomap.elements;

	/*
	 * Disable the range.
	 */
	bitmap_set_range(&task->arch.iomap, (size_t) ioaddr, size);

	/*
	 * Increment I/O Permission bitmap generation counter.
	 */
	task->arch.iomapver++;

	return 0;
}

/** @}
 */
