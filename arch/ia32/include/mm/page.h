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

#ifndef __ia32_PAGE_H__
#define __ia32_PAGE_H__

#include <arch/types.h>
#include <arch/mm/frame.h>

#define PAGE_SIZE	FRAME_SIZE

#define KA2PA(x)	((x) - 0x80000000)
#define PA2KA(x)	((x) + 0x80000000)

/*
 * Implementation of generic 4-level page table interface.
 * IA-32 has 2-level page tables, so PTL1 and PTL2 are left out.
 */
#define PTL0_INDEX_ARCH(vaddr)	(((vaddr)>>22)&0x3ff)
#define PTL1_INDEX_ARCH(vaddr)	0
#define PTL2_INDEX_ARCH(vaddr)	0
#define PTL3_INDEX_ARCH(vaddr)	(((vaddr)>>12)&0x3ff)

#define GET_PTL1_ADDRESS_ARCH(ptl0, i)		((pte_t *)((((pte_t *)(ptl0))[(i)].frame_address)<<12))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i)		(ptl1)
#define GET_PTL3_ADDRESS_ARCH(ptl2, i)		(ptl2)
#define GET_FRAME_ADDRESS_ARCH(ptl3, i)		((__address)((((pte_t *)(ptl3))[(i)].frame_address)<<12))


struct page_specifier {
	unsigned present : 1;
	unsigned writeable : 1;
	unsigned uaccessible : 1;
	unsigned page_write_through : 1;
	unsigned page_cache_disable : 1;
	unsigned accessed : 1;
	unsigned dirty : 1;
	unsigned : 2;
	unsigned avl : 3;
	unsigned frame_address : 20;
} __attribute__ ((packed));

typedef struct page_specifier	pte_t;

extern void page_arch_init(void);

#endif
