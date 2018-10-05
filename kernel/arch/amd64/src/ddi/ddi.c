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
#include <assert.h>
#include <proc/task.h>
#include <adt/bitmap.h>
#include <mm/slab.h>
#include <arch/pm.h>
#include <errno.h>
#include <arch/cpu.h>
#include <cpu.h>
#include <arch.h>
#include <align.h>
#include <stdbool.h>

/** Install I/O Permission bitmap.
 *
 * Current task's I/O permission bitmap, if any, is installed
 * in the current CPU's TSS.
 *
 * Interrupts must be disabled prior this call.
 *
 */
void io_perm_bitmap_install(void)
{
	/* First, copy the I/O Permission Bitmap. */
	irq_spinlock_lock(&TASK->lock, false);

	size_t ver = TASK->arch.iomapver;
	size_t elements = TASK->arch.iomap.elements;

	if (elements > 0) {
		assert(TASK->arch.iomap.bits);

		bitmap_t iomap;
		bitmap_initialize(&iomap, TSS_IOMAP_SIZE * 8,
		    CPU->arch.tss->iomap);
		bitmap_copy(&iomap, &TASK->arch.iomap, elements);

		/*
		 * Set the trailing bits in the last byte of the map to disable
		 * I/O access.
		 */
		bitmap_set_range(&iomap, elements,
		    ALIGN_UP(elements, 8) - elements);

		/*
		 * It is safe to set the trailing eight bits because of the
		 * extra convenience byte in TSS_IOMAP_SIZE.
		 */
		bitmap_set_range(&iomap, ALIGN_UP(elements, 8), 8);
	}

	irq_spinlock_unlock(&TASK->lock, false);

	/*
	 * Second, adjust TSS segment limit.
	 * Take the extra ending byte with all bits set into account.
	 */
	ptr_16_64_t cpugdtr;
	gdtr_store(&cpugdtr);

	descriptor_t *gdt_p = (descriptor_t *) cpugdtr.base;
	size_t size = bitmap_size(elements);
	gdt_tss_setlimit(&gdt_p[TSS_DES], TSS_BASIC_SIZE + size);
	gdtr_load(&cpugdtr);

	/*
	 * Before we load new TSS limit, the current TSS descriptor
	 * type must be changed to describe inactive TSS.
	 */
	tss_descriptor_t *tss_desc = (tss_descriptor_t *) &gdt_p[TSS_DES];
	tss_desc->type = AR_TSS;
	tr_load(GDT_SELECTOR(TSS_DES));

	/*
	 * Update the generation count so that faults caused by
	 * early accesses can be serviced.
	 */
	CPU->arch.iomapver_copy = ver;
}

/** @}
 */
