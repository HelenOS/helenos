/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup sparc32mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <arch/mm/page_fault.h>
#include <arch/mm/tlb.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <typedefs.h>
#include <align.h>
#include <config.h>
#include <func.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <debug.h>
#include <memstr.h>
#include <print.h>
#include <interrupt.h>
#include <macros.h>

void page_arch_init(void)
{
	int flags = PAGE_CACHEABLE | PAGE_EXEC;
	page_mapping_operations = &pt_mapping_operations;
	
	page_table_lock(AS_KERNEL, true);
	
	/* Kernel identity mapping */
	// FIXME:
	// We need to consider the possibility that
	// identity_base > identity_size and physmem_end.
	// This might lead to overflow if identity_size is too big.
	for (uintptr_t cur = PHYSMEM_START_ADDR;
	    cur < min(KA2PA(config.identity_base) +
	    config.identity_size, config.physmem_end);
	    cur += FRAME_SIZE)
		page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, flags);
	
	page_table_unlock(AS_KERNEL, true);
	as_switch(NULL, AS_KERNEL);
	
	/* Switch MMU to new context table */
	asi_u32_write(ASI_MMUREGS, MMU_CONTEXT_TABLE, KA2PA(as_context_table) >> 4);
}

void page_fault(unsigned int n __attribute__((unused)), istate_t *istate)
{
	uint32_t fault_status = asi_u32_read(ASI_MMUREGS, MMU_FAULT_STATUS);
	uintptr_t fault_address = asi_u32_read(ASI_MMUREGS, MMU_FAULT_ADDRESS);
	mmu_fault_status_t *fault = (mmu_fault_status_t *) &fault_status;
	mmu_fault_type_t type = (mmu_fault_type_t) fault->at;
	
	if ((type == FAULT_TYPE_LOAD_USER_DATA) ||
	    (type == FAULT_TYPE_LOAD_SUPERVISOR_DATA))
		as_page_fault(fault_address, PF_ACCESS_READ, istate);

	if ((type == FAULT_TYPE_EXECUTE_USER) ||
	    (type == FAULT_TYPE_EXECUTE_SUPERVISOR))
		as_page_fault(fault_address, PF_ACCESS_EXEC, istate);

	if ((type == FAULT_TYPE_STORE_USER_DATA) ||
	    (type == FAULT_TYPE_STORE_USER_INSTRUCTION) ||
	    (type == FAULT_TYPE_STORE_SUPERVISOR_INSTRUCTION) ||
	    (type == FAULT_TYPE_STORE_SUPERVISOR_DATA))
		as_page_fault(fault_address, PF_ACCESS_WRITE, istate);
}

/** @}
 */
