/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_ddi
 * @{
 */
/** @file
 */

#include <ddi/ddi.h>
#include <arch/ddi/ddi.h>
#include <assert.h>
#include <proc/task.h>
#include <stddef.h>
#include <adt/bitmap.h>
#include <mm/slab.h>
#include <arch/pm.h>
#include <errno.h>
#include <arch/cpu.h>
#include <cpu.h>
#include <arch.h>
#include <align.h>

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
	ptr_16_32_t cpugdtr;
	gdtr_store(&cpugdtr);

	descriptor_t *gdt_p = (descriptor_t *) cpugdtr.base;
	size_t size = bitmap_size(elements);
	gdt_setlimit(&gdt_p[TSS_DES], TSS_BASIC_SIZE + size);
	gdtr_load(&cpugdtr);

	/*
	 * Before we load new TSS limit, the current TSS descriptor
	 * type must be changed to describe inactive TSS.
	 */
	gdt_p[TSS_DES].access = AR_PRESENT | AR_TSS | DPL_KERNEL;
	tr_load(GDT_SELECTOR(TSS_DES));

	/*
	 * Update the generation count so that faults caused by
	 * early accesses can be serviced.
	 */
	CPU->arch.iomapver_copy = ver;
}

/** @}
 */
