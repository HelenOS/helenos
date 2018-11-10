/*
 * Copyright (c) 2006 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
