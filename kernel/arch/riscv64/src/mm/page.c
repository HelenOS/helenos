/*
 * Copyright (c) 2016 Martin Decky
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

/** @addtogroup riscv64mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/frame.h>
#include <arch/cpu.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <align.h>
#include <config.h>
#include <halt.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <debug.h>
#include <print.h>
#include <interrupt.h>

void page_arch_init(void)
{
	if (config.cpu_active == 1) {
		page_mapping_operations = &pt_mapping_operations;
		
		page_table_lock(AS_KERNEL, true);
		
		/*
		 * PA2KA(identity) mapping for all low-memory frames.
		 */
		for (uintptr_t cur = 0;
		    cur < min(config.identity_size, config.physmem_end);
		    cur += FRAME_SIZE)
			page_mapping_insert(AS_KERNEL, PA2KA(cur), cur,
			    PAGE_GLOBAL | PAGE_CACHEABLE | PAGE_EXEC | PAGE_WRITE | PAGE_READ);
		
		page_table_unlock(AS_KERNEL, true);
		
		// FIXME: register page fault extension handler
		
		write_satp((uintptr_t) AS_KERNEL->genarch.page_table);
		
		/* The boot page table is no longer needed. */
		// FIXME: frame_mark_available(pt_frame, 1);
	}
}

void page_fault(unsigned int n __attribute__((unused)), istate_t *istate)
{
}

void write_satp(uintptr_t ptl0)
{
	uint64_t satp = ((ptl0 >> FRAME_WIDTH) & SATP_PFN_MASK) |
	    SATP_MODE_SV48;
	
	asm volatile (
		"csrw sptbr, %[satp]\n"
		:: [satp] "r" (satp)
	);
}

/** @}
 */
