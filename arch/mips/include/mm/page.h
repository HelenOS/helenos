/*
 * Copyright (C) 2003-2004 Jakub Jermar
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

#ifndef __mips_PAGE_H__
#define __mips_PAGE_H__

#define PAGE_SIZE	FRAME_SIZE

#ifndef __ASM__
#  define KA2PA(x)	(((__address) (x)) - 0x80000000)
#  define PA2KA(x)	(((__address) (x)) + 0x80000000)
#else
#  define KA2PA(x)	((x) - 0x80000000)
#  define PA2KA(x)	((x) + 0x80000000)
#endif

/*
 * Implementation of generic 4-level page table interface.
 * NOTE: this implementation is under construction
 * 
 * Page table layout:
 * - 32-bit virtual addresses
 * - Offset is 14 bits => pages are 16K long
 * - PTE's use the same format as CP0 EntryLo[01] registers => PTE is therefore 4 bytes long
 * - PTL0 has 64 entries (6 bits)
 * - PTL1 is not used
 * - PTL2 is not used
 * - PTL3 has 4096 entries (12 bits)
 */
 
#define PTL0_INDEX_ARCH(vaddr)  ((vaddr)>>26) 
#define PTL1_INDEX_ARCH(vaddr)  0
#define PTL2_INDEX_ARCH(vaddr)  0
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr)>>14)&0xfff)

#define GET_PTL0_ADDRESS_ARCH()			(PTL0)
#define SET_PTL0_ADDRESS_ARCH(ptl0)		(PTL0 = (pte_t *)(ptl0))

#define GET_PTL1_ADDRESS_ARCH(ptl0, i)		(((pte_t *)(ptl0))[(i)].pfn<<14)
#define GET_PTL2_ADDRESS_ARCH(ptl1, i)		(ptl1)
#define GET_PTL3_ADDRESS_ARCH(ptl2, i)		(ptl2)
#define GET_FRAME_ADDRESS_ARCH(ptl3, i)		(((pte_t *)(ptl3))[(i)].pfn<<14)

#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a)	(((pte_t *)(ptl0))[(i)].pfn = (a)>>14)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a)	(((pte_t *)(ptl3))[(i)].pfn = (a)>>14)

#define GET_PTL1_FLAGS_ARCH(ptl0, i)		get_pt_flags((pte_t *)(ptl0), (index_t)(i))
#define GET_PTL2_FLAGS_ARCH(ptl1, i)		PAGE_PRESENT
#define GET_PTL3_FLAGS_ARCH(ptl2, i)		PAGE_PRESENT
#define GET_FRAME_FLAGS_ARCH(ptl3, i)		get_pt_flags((pte_t *)(ptl3), (index_t)(i))

#define SET_PTL1_FLAGS_ARCH(ptl0, i, x)		set_pt_flags((pte_t *)(ptl0), (index_t)(i), (x))
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x)	set_pt_flags((pte_t *)(ptl3), (index_t)(i), (x))

#ifndef __ASM__

#include <arch/mm/tlb.h>
#include <mm/page.h>
#include <arch/mm/frame.h>
#include <arch/types.h>

static inline int get_pt_flags(pte_t *pt, index_t i)
{
	pte_t *p = &pt[i];
	
	return (
		((p->c>PAGE_UNCACHED)<<PAGE_CACHEABLE_SHIFT) |
		((!p->v)<<PAGE_PRESENT_SHIFT) |
		(1<<PAGE_USER_SHIFT) |
		(1<<PAGE_READ_SHIFT) |
		((p->d)<<PAGE_WRITE_SHIFT) |
		(1<<PAGE_EXEC_SHIFT)
	);
		
}

static inline void set_pt_flags(pte_t *pt, index_t i, int flags)
{
	pte_t *p = &pt[i];
	
	p->c = (flags & PAGE_CACHEABLE) != 0 ? PAGE_CACHEABLE_EXC_WRITE : PAGE_UNCACHED;
	p->v = !(flags & PAGE_NOT_PRESENT);
	p->d = (flags & PAGE_WRITE) != 0;
}

extern void page_arch_init(void);

extern pte_t *PTL0;

#endif /* __ASM__ */

#endif
