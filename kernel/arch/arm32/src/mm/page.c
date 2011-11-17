/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32mm
 * @{
 */
/** @file
 *  @brief Paging related functions.
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <arch/mm/frame.h>
#include <align.h>
#include <config.h>
#include <arch/exception.h>
#include <typedefs.h>
#include <interrupt.h>
#include <macros.h>

/** Initializes page tables.
 *
 * 1:1 virtual-physical mapping is created in kernel address space. Mapping
 * for table with exception vectors is also created.
 */
void page_arch_init(void)
{
	int flags = PAGE_CACHEABLE;
	page_mapping_operations = &pt_mapping_operations;

	page_table_lock(AS_KERNEL, true);
	
	uintptr_t cur;
	/* Kernel identity mapping */
	for (cur = PHYSMEM_START_ADDR;
	    cur < min(config.identity_base, last_frame); cur += FRAME_SIZE)
		page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, flags);
	
	/* Create mapping for exception table at high offset */
#ifdef HIGH_EXCEPTION_VECTORS
	// XXX: fixme to use proper non-identity page
	void *virtaddr = frame_alloc(ONE_FRAME, FRAME_KA);
	page_mapping_insert(AS_KERNEL, EXC_BASE_ADDRESS, KA2PA(virtaddr),
	    flags);
#else
#error "Only high exception vector supported now"
#endif
	cur = ALIGN_DOWN(0x50008010, FRAME_SIZE);
	page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, flags);

	page_table_unlock(AS_KERNEL, true);
	
	as_switch(NULL, AS_KERNEL);
	
	boot_page_table_free();
}

/** Maps device into the kernel space.
 *
 * Maps physical address of device into kernel virtual address space (so it can
 * be accessed only by kernel through virtual address).
 *
 * @param physaddr Physical address where device is connected.
 * @param size Length of area where device is present.
 *
 * @return Virtual address where device will be accessible.
 */
uintptr_t hw_map(uintptr_t physaddr, size_t size)
{
	if (last_frame + ALIGN_UP(size, PAGE_SIZE) >
	    KA2PA(KERNEL_ADDRESS_SPACE_END_ARCH)) {
		panic("Unable to map physical memory %p (%d bytes).",
		    (void *) physaddr, size);
	}
	
	uintptr_t virtaddr = PA2KA(last_frame);
	pfn_t i;

	page_table_lock(AS_KERNEL, true);
	for (i = 0; i < ADDR2PFN(ALIGN_UP(size, PAGE_SIZE)); i++) {
		page_mapping_insert(AS_KERNEL, virtaddr + PFN2ADDR(i),
		    physaddr + PFN2ADDR(i),
		    PAGE_NOT_CACHEABLE | PAGE_READ | PAGE_WRITE | PAGE_KERNEL);
	}
	page_table_unlock(AS_KERNEL, true);
	
	last_frame = ALIGN_UP(last_frame + size, FRAME_SIZE);
	return virtaddr;
}

/** @}
 */
