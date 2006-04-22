/*
 * Copyright (C) 2006 Jakub Jermar
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

#include <ddi/ddi.h>
#include <proc/task.h>
#include <arch/types.h>
#include <typedefs.h>
#include <adt/bitmap.h>
#include <mm/slab.h>
#include <arch/pm.h>
#include <errno.h>
#include <arch/cpu.h>

/** Enable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task Task.
 * @param ioaddr Startign I/O space address.
 * @param size Size of the enabled I/O range.
 *
 * @return 0 on success or an error code from errno.h.
 */
int ddi_enable_iospace_arch(task_t *task, __address ioaddr, size_t size)
{
	count_t bits;

	bits = ioaddr + size;
	if (bits > IO_PORTS)
		return ENOENT;

	if (task->arch.iomap.bits < bits) {
		bitmap_t oldiomap;
		__u8 *newmap;
	
		/*
		 * The I/O permission bitmap is too small and needs to be grown.
		 */
		
		newmap = (__u8 *) malloc(BITS2BYTES(bits), FRAME_ATOMIC);
		if (!newmap)
			return ENOMEM;
		
		bitmap_initialize(&oldiomap, task->arch.iomap.map, task->arch.iomap.bits);
		bitmap_initialize(&task->arch.iomap, newmap, bits);

		/*
		 * Mark the new range inaccessible.
		 */
		bitmap_set_range(&task->arch.iomap, oldiomap.bits, bits - oldiomap.bits);

		/*
		 * In case there really existed smaller iomap,
		 * copy its contents and deallocate it.
		 */		
		if (oldiomap.bits) {
			bitmap_copy(&task->arch.iomap, &oldiomap, task->arch.iomap.bits);
			free(oldiomap.map);
		}
	}

	/*
	 * Enable the range and we are done.
	 */
	bitmap_clear_range(&task->arch.iomap, (index_t) ioaddr, (count_t) size);

	return 0;
}

/** Enable/disable interrupts form syscall
 *
 * @param enable If non-zero, interrupts are enabled, otherwise disabled
 * @param flags CP0 flags register
 */
__native ddi_int_control_arch(__native enable, __native *flags)
{
	if (enable)
		*flags |= RFLAGS_IF;
	else
		*flags &= ~RFLAGS_IF;
	return 0;
}
