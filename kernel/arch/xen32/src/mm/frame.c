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

#define L0_PT_SHIFT	10
#define L3_PT_SHIFT	0

#define L0_PT_ENTRIES	1024
#define L3_PT_ENTRIES	1024

#define L0_INDEX_MASK			(L0_PT_ENTRIES - 1)
#define L3_INDEX_MASK			(L3_PT_ENTRIES - 1)

#define PFN2PTL0_INDEX(pfn)	((pfn >> L0_PT_SHIFT) & L0_INDEX_MASK)
#define PFN2PTL3_INDEX(pfn)	((pfn >> L3_PT_SHIFT) & L3_INDEX_MASK)

#define PAGE_MASK	(~(PAGE_SIZE - 1))

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

#define L0_PROT	(_PAGE_PRESENT | _PAGE_ACCESSED)
#define L3_PROT	(_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED)

void frame_arch_init(void)
{
	if (config.cpu_active == 1) {
		/* The only memory zone starts just after page table */
		pfn_t start = ADDR2PFN(ALIGN_UP(KA2PA(start_info.ptl0), PAGE_SIZE)) + start_info.pt_frames;
		size_t size = start_info.frames - start;
		
		/* Create identity mapping */
		pfn_t phys;
		count_t count = 0;
		for (phys = start; phys < start + size; phys++) {
			mmu_update_t updates[L3_PT_ENTRIES];
			pfn_t virt = ADDR2PFN(PA2KA(PFN2ADDR(phys)));
			
			size_t ptl0_index = PFN2PTL0_INDEX(virt);
			size_t ptl3_index = PFN2PTL3_INDEX(virt);
			
			pte_t *ptl3 = (pte_t *) PFN2ADDR(start_info.ptl0[ptl0_index].frame_address);
			
			if (ptl3 == 0) {
				mmuext_op_t mmu_ext;
				
				pfn_t virt2 = ADDR2PFN(PA2KA(PFN2ADDR(start)));
				
				/* New L1 page table entry needed */
				memsetb(PFN2ADDR(virt2), PAGE_SIZE, 0);
				
				size_t ptl0_index2 = PFN2PTL0_INDEX(virt2);
				size_t ptl3_index2 = PFN2PTL3_INDEX(virt2);
				pte_t *ptl3_2 = (pte_t *) PFN2ADDR(start_info.ptl0[ptl0_index2].frame_address);
				
				if (ptl3_2 == 0)
					panic("Unable to find page table reference");
				
				updates[count].ptr = (uintptr_t) &ptl3_2[ptl3_index2];
				updates[count].val = PA2MA(PFN2ADDR(start)) | L0_PROT;
				if (xen_mmu_update(updates, count + 1, NULL, DOMID_SELF) < 0)
					panic("Unable to map new page table");
				count = 0;
				
				mmu_ext.cmd = MMUEXT_PIN_L1_TABLE;
				mmu_ext.arg1.mfn = ADDR2PFN(PA2MA(PFN2ADDR(start)));
				if (xen_mmuext_op(&mmu_ext, 1, NULL, DOMID_SELF) < 0)
					panic("Error pinning new page table");
				
				pte_t *ptl0 = (pte_t *) PA2MA(KA2PA(start_info.ptl0));
				
				updates[count].ptr = (uintptr_t) &ptl0[ptl0_index];
				updates[count].val = PA2MA(PFN2ADDR(start)) | L3_PROT;
				if (xen_mmu_update(updates, count + 1, NULL, DOMID_SELF) < 0)
					panic("Unable to update PTE for page table");
				count = 0;
				
				ptl3 = (pte_t *) PFN2ADDR(start_info.ptl0[ptl0_index].frame_address);
				start++;
				size--;
			}
			
			updates[count].ptr = (uintptr_t) &ptl3[ptl3_index];
			updates[count].val = PA2MA(PFN2ADDR(phys)) | L3_PROT;
			count++;
			
			if ((count == L3_PT_ENTRIES) || (phys + 1 == start + size)) {
				if (xen_mmu_update(updates, count, NULL, DOMID_SELF) < 0)
					panic("Unable to update PTE");
				count = 0;
			}
		}
		
		zone_create(start, size, start, 0);
		last_frame = start + size;
	}
}

/** @}
 */
