/*
 * Copyright (C) 2005 Martin Decky
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

#ifndef __amd64_PAGE_H__
#define __amd64_PAGE_H__

#ifndef __ASM__
#  include <mm/page.h>
#  include <arch/mm/frame.h>
#  include <arch/types.h>
#endif

#define PAGE_SIZE	FRAME_SIZE

#ifndef __ASM__
# define KA2PA(x)      (((__address) (x)) - 0xffffffff80000000)
# define PA2KA(x)      (((__address) (x)) + 0xffffffff80000000)
#else
# define KA2PA(x)      ((x) - 0xffffffff80000000)
# define PA2KA(x)      ((x) + 0xffffffff80000000)
#endif

#define PTL0_INDEX_ARCH(vaddr)	(((vaddr)>>39)&0x1ff)
#define PTL1_INDEX_ARCH(vaddr)	(((vaddr)>>30)&0x1ff)
#define PTL2_INDEX_ARCH(vaddr)	(((vaddr)>>21)&0x1ff)
#define PTL3_INDEX_ARCH(vaddr)	(((vaddr)>>12)&0x1ff)

#define GET_PTL0_ADDRESS_ARCH()			((pte_t *) read_cr3())
#define GET_PTL1_ADDRESS_ARCH(ptl0, i)		((pte_t *) ((((__u64) ((pte_t *)(ptl0))[(i)].addr_12_31)<<12) | (((__u64) ((pte_t *)(ptl0))[(i)].addr_32_51)<<32 )))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i)		((pte_t *) ((((__u64) ((pte_t *)(ptl1))[(i)].addr_12_31)<<12) | (((__u64) ((pte_t *)(ptl1))[(i)].addr_32_51)<<32 )))
#define GET_PTL3_ADDRESS_ARCH(ptl2, i)		((pte_t *) ((((__u64) ((pte_t *)(ptl2))[(i)].addr_12_31)<<12) | (((__u64) ((pte_t *)(ptl2))[(i)].addr_32_51)<<32 )))
#define GET_FRAME_ADDRESS_ARCH(ptl3, i)		((__address *) ((((__u64) ((pte_t *)(ptl3))[(i)].addr_12_31)<<12) | (((__u64) ((pte_t *)(ptl3))[(i)].addr_32_51)<<32 )))

#define SET_PTL0_ADDRESS_ARCH(ptl0)		(write_cr3((__address) (ptl0)))
#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a)	set_pt_addr((pte_t *)(ptl0), (index_t)(i), a)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)       set_pt_addr((pte_t *)(ptl1), (index_t)(i), a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)       set_pt_addr((pte_t *)(ptl2), (index_t)(i), a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a)	set_pt_addr((pte_t *)(ptl3), (index_t)(i), a)

#define GET_PTL1_FLAGS_ARCH(ptl0, i)		get_pt_flags((pte_t *)(ptl0), (index_t)(i))
#define GET_PTL2_FLAGS_ARCH(ptl1, i)		get_pt_flags((pte_t *)(ptl1), (index_t)(i))
#define GET_PTL3_FLAGS_ARCH(ptl2, i)		get_pt_flags((pte_t *)(ptl2), (index_t)(i))
#define GET_FRAME_FLAGS_ARCH(ptl3, i)		get_pt_flags((pte_t *)(ptl3), (index_t)(i))

#define SET_PTL1_FLAGS_ARCH(ptl0, i, x)		set_pt_flags((pte_t *)(ptl0), (index_t)(i), (x))
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)         set_pt_flags((pte_t *)(ptl1), (index_t)(i), (x))
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)         set_pt_flags((pte_t *)(ptl2), (index_t)(i), (x))
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x)	set_pt_flags((pte_t *)(ptl3), (index_t)(i), (x))

#ifndef __ASM__

typedef struct page_specifier pte_t;

struct page_specifier {
	unsigned present : 1;
	unsigned writeable : 1;
	unsigned uaccessible : 1;
	unsigned page_write_through : 1;
	unsigned page_cache_disable : 1;
	unsigned accessed : 1;
	unsigned dirty : 1;
	unsigned unused: 1;
	unsigned global : 1;
	unsigned avl : 3;
	unsigned addr_12_31 : 30;
	unsigned addr_32_51 : 21;
	unsigned no_execute : 1;
} __attribute__ ((packed));

static inline int get_pt_flags(pte_t *pt, index_t i)
{
	pte_t *p = &pt[i];
	
	return (
		(!p->page_cache_disable)<<PAGE_CACHEABLE_SHIFT |
		(!p->present)<<PAGE_PRESENT_SHIFT |
		p->uaccessible<<PAGE_USER_SHIFT |
		1<<PAGE_READ_SHIFT |
		p->writeable<<PAGE_WRITE_SHIFT |
		(!p->no_execute)<<PAGE_EXEC_SHIFT
	);
}

static inline void set_pt_addr(pte_t *pt, index_t i, __address a)
{
	pte_t *p = &pt[i];

	p->addr_12_31 = (a >> 12) & 0xfffff;
	p->addr_32_51 = a >> 32;
}

static inline void set_pt_flags(pte_t *pt, index_t i, int flags)
{
	pte_t *p = &pt[i];
	
	p->page_cache_disable = !(flags & PAGE_CACHEABLE);
	p->present = !(flags & PAGE_NOT_PRESENT);
	p->uaccessible = (flags & PAGE_USER) != 0;
	p->writeable = (flags & PAGE_WRITE) != 0;
	p->no_execute = (flags & PAGE_EXEC) == 0;
}

extern void page_arch_init(void);

#endif

#endif
