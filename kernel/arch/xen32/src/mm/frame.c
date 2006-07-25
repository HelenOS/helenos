/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

/** @addtogroup xen32mm
 * @{
 */
/** @file
 * @ingroup xen32mm
 */

#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <mm/as.h>
#include <config.h>
#include <arch/boot/boot.h>
#include <arch/hypercall.h>
#include <panic.h>
#include <debug.h>
#include <align.h>
#include <macros.h>

#include <console/cmd.h>
#include <console/kconsole.h>

uintptr_t last_frame = 0;

#define L1_PT_SHIFT	10
#define L2_PT_SHIFT	0

#define L1_OFFSET_MASK			0x3ff
#define L2_OFFSET_MASK			0x3ff

#define PFN2PTL1_OFFSET(pfn)	((pfn >> L1_PT_SHIFT) & L1_OFFSET_MASK)
#define PFN2PTL2_OFFSET(pfn)	((pfn >> L2_PT_SHIFT) & L2_OFFSET_MASK)

#define PAGE_MASK	(~(PAGE_SIZE - 1))

#define PTE2ADDR(pte)	(pte & PAGE_MASK)

#define _PAGE_PRESENT	0x001UL
#define _PAGE_RW		0x002UL
#define _PAGE_USER		0x004UL
#define _PAGE_PWT		0x008UL
#define _PAGE_PCD		0x010UL
#define _PAGE_ACCESSED	0x020UL
#define _PAGE_DIRTY		0x040UL
#define _PAGE_PAT		0x080UL
#define _PAGE_PSE		0x080UL
#define _PAGE_GLOBAL	0x100UL

#define L1_PROT	(_PAGE_PRESENT | _PAGE_ACCESSED)
#define L2_PROT	(_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED)

void frame_arch_init(void)
{
	if (config.cpu_active == 1) {
		/* The only memory zone starts just after page table */
		pfn_t start = ADDR2PFN(ALIGN_UP(KA2PA(start_info.pt_base), PAGE_SIZE)) + start_info.nr_pt_frames;
		size_t size = start_info.nr_pages - start;
		
		/* Create identity mapping */
		pfn_t phys;
		for (phys = start; phys < start + size; phys++) {
			mmu_update_t updates[1];
			pfn_t virt = ADDR2PFN(PA2KA(PFN2ADDR(phys)));
			
			size_t ptl1_offset = PFN2PTL1_OFFSET(virt);
			size_t ptl2_offset = PFN2PTL2_OFFSET(virt);
			
			unsigned long *ptl2_base = (unsigned long *) PTE2ADDR(start_info.pt_base[ptl1_offset]);
			
			if (ptl2_base == 0) {
				mmuext_op_t mmu_ext;
				
				pfn_t virt2 = ADDR2PFN(PA2KA(PFN2ADDR(start)));
				
				/* New L1 page table entry needed */
				memsetb(PFN2ADDR(virt2), PAGE_SIZE, 0);
				
				size_t ptl1_offset2 = PFN2PTL1_OFFSET(virt2);
				size_t ptl2_offset2 = PFN2PTL2_OFFSET(virt2);
				unsigned long *ptl2_base2 = (unsigned long *) PTE2ADDR(start_info.pt_base[ptl1_offset2]);
				
				if (ptl2_base2 == 0)
					panic("Unable to find page table reference");
				
				updates[0].ptr = (uintptr_t) &ptl2_base2[ptl2_offset2];
				updates[0].val = PFN2ADDR(start_info.mfn_list[start]) | L1_PROT;
				if (xen_mmu_update(updates, 1, NULL, DOMID_SELF) < 0)
					panic("Unable to map new page table");
				
				mmu_ext.cmd = MMUEXT_PIN_L1_TABLE;
				mmu_ext.arg1.mfn = start_info.mfn_list[start];
				if (xen_mmuext_op(&mmu_ext, 1, NULL, DOMID_SELF) < 0)
					panic("Error pinning new page table");
				
				unsigned long *ptl0 = (unsigned long *) PFN2ADDR(start_info.mfn_list[ADDR2PFN(KA2PA(start_info.pt_base))]);
				
				updates[0].ptr = (uintptr_t) &ptl0[ptl1_offset];
				updates[0].val = PFN2ADDR(start_info.mfn_list[start]) | L2_PROT;
				if (xen_mmu_update(updates, 1, NULL, DOMID_SELF) < 0)
					panic("Unable to update PTE for page table");
				
				ptl2_base = (unsigned long *) PTE2ADDR(start_info.pt_base[ptl1_offset]);
				start++;
				size--;
			}
			
			updates[0].ptr = (uintptr_t) &ptl2_base[ptl2_offset];
			updates[0].val = PFN2ADDR(start_info.mfn_list[phys]) | L2_PROT;
			if (xen_mmu_update(updates, 1, NULL, DOMID_SELF) < 0)
				panic("Unable to update PTE");
		}
		
		zone_create(start, size, start, 0);
		last_frame = start + size;
	}
}

/** @}
 */
