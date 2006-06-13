/*
 * Copyright (C) 2005 Jakub Jermar
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

 /** @addtogroup sparc64mm	
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <arch/mm/tlb.h>
#include <genarch/mm/page_ht.h>
#include <mm/frame.h>
#include <bitops.h>
#include <debug.h>

void page_arch_init(void)
{
	page_mapping_operations = &ht_mapping_operations;
}

__address hw_map(__address physaddr, size_t size)
{
	unsigned int order;
	int i;

	struct {
		int pagesize;
		size_t increment;
		count_t count;
	} sizemap[] = {
		{ PAGESIZE_8K, 0, 1 },			/* 8K */
		{ PAGESIZE_8K, PAGE_SIZE, 2 },		/* 16K */
		{ PAGESIZE_8K, PAGE_SIZE, 4 },		/* 32K */
		{ PAGESIZE_64K, 0, 1},			/* 64K */
		{ PAGESIZE_64K, 8*PAGE_SIZE, 2 },	/* 128K */
		{ PAGESIZE_64K, 8*PAGE_SIZE, 4 },	/* 256K */
		{ PAGESIZE_512K, 0, 1 },		/* 512K */
		{ PAGESIZE_512K, 64*PAGE_SIZE, 2 },	/* 1M */
		{ PAGESIZE_512K, 64*PAGE_SIZE, 4 },	/* 2M */
		{ PAGESIZE_4M, 0, 1 }			/* 4M */
	};
	
	ASSERT(size <= 4*1024*1024);
	
	if (size <= FRAME_SIZE)
		order = 0;
	else
		order = (fnzb32(size - 1) + 1) - FRAME_WIDTH;
	
	__address virtaddr = PA2KA(PFN2ADDR(frame_alloc(order, FRAME_KA)));

	for (i = 0; i < sizemap[order].count; i++)
		dtlb_insert_mapping(virtaddr + i*sizemap[order].increment,
				    physaddr + i*sizemap[order].increment,
				    sizemap[order].pagesize, true, false);
	
	return virtaddr;
}

 /** @}
 */

