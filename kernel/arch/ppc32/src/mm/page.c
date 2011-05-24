/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup ppc32mm
 * @{
 */
/** @file
 */

#include <genarch/mm/page_pt.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <align.h>
#include <config.h>

void page_arch_init(void)
{
	if (config.cpu_active == 1)
		page_mapping_operations = &pt_mapping_operations;
	as_switch(NULL, AS_KERNEL);
}

uintptr_t hw_map(uintptr_t physaddr, size_t size)
{
	if (last_frame + ALIGN_UP(size, PAGE_SIZE) >
	    KA2PA(KERNEL_ADDRESS_SPACE_END_ARCH))
		panic("Unable to map physical memory %p (%zu bytes).",
		    (void *) physaddr, size);
	
	uintptr_t virtaddr = PA2KA(last_frame);
	pfn_t i;
	page_table_lock(AS_KERNEL, true);
	for (i = 0; i < ADDR2PFN(ALIGN_UP(size, PAGE_SIZE)); i++)
		page_mapping_insert(AS_KERNEL, virtaddr + PFN2ADDR(i),
		    physaddr + PFN2ADDR(i), PAGE_NOT_CACHEABLE | PAGE_WRITE);
	page_table_unlock(AS_KERNEL, true);
	
	last_frame = ALIGN_UP(last_frame + size, FRAME_SIZE);
	
	return virtaddr;
}

/** @}
 */
