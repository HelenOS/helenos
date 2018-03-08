/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup amd64mm
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_PAGE_H_
#define KERN_amd64_PAGE_H_

#include <arch/mm/frame.h>
#include <trace.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#ifdef MEMORY_MODEL_kernel

#ifndef __ASSEMBLER__

#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffffffff80000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT64_C(0xffffffff80000000))

#else /* __ASSEMBLER__ */

#define KA2PA(x)  ((x) - 0xffffffff80000000)
#define PA2KA(x)  ((x) + 0xffffffff80000000)

#endif /* __ASSEMBLER__ */

#endif /* MEMORY_MODEL_kernel */

#ifdef MEMORY_MODEL_large

#ifndef __ASSEMBLER__

#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffff800000000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT64_C(0xffff800000000000))

#else /* __ASSEMBLER__ */

#define KA2PA(x)  ((x) - 0xffff800000000000)
#define PA2KA(x)  ((x) + 0xffff800000000000)

#endif /* __ASSEMBLER__ */

#endif /* MEMORY_MODEL_large */

/* Number of entries in each level. */
#define PTL0_ENTRIES_ARCH  512
#define PTL1_ENTRIES_ARCH  512
#define PTL2_ENTRIES_ARCH  512
#define PTL3_ENTRIES_ARCH  512

/* Page table sizes for each level. */
#define PTL0_FRAMES_ARCH  1
#define PTL1_FRAMES_ARCH  1
#define PTL2_FRAMES_ARCH  1
#define PTL3_FRAMES_ARCH  1

/* Macros calculating indices into page tables in each level. */
#define PTL0_INDEX_ARCH(vaddr)  (((vaddr) >> 39) & 0x1ffU)
#define PTL1_INDEX_ARCH(vaddr)  (((vaddr) >> 30) & 0x1ffU)
#define PTL2_INDEX_ARCH(vaddr)  (((vaddr) >> 21) & 0x1ffU)
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> 12) & 0x1ffU)

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
	((pte_t *) ((((uint64_t) ((pte_t *) (ptl0))[(i)].addr_12_31) << 12) | \
	    (((uint64_t) ((pte_t *) (ptl0))[(i)].addr_32_51) << 32)))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
	((pte_t *) ((((uint64_t) ((pte_t *) (ptl1))[(i)].addr_12_31) << 12) | \
	    (((uint64_t) ((pte_t *) (ptl1))[(i)].addr_32_51) << 32)))
#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
	((pte_t *) ((((uint64_t) ((pte_t *) (ptl2))[(i)].addr_12_31) << 12) | \
	    (((uint64_t) ((pte_t *) (ptl2))[(i)].addr_32_51) << 32)))
#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
	((uintptr_t *) \
	    ((((uint64_t) ((pte_t *) (ptl3))[(i)].addr_12_31) << 12) | \
	    (((uint64_t) ((pte_t *) (ptl3))[(i)].addr_32_51) << 32)))

/* Set PTE address accessors for each level. */
#define SET_PTL0_ADDRESS_ARCH(ptl0) \
	(write_cr3((uintptr_t) (ptl0)))
#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
	set_pt_addr((pte_t *) (ptl0), (size_t) (i), a)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a) \
	set_pt_addr((pte_t *) (ptl1), (size_t) (i), a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a) \
	set_pt_addr((pte_t *) (ptl2), (size_t) (i), a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
	set_pt_addr((pte_t *) (ptl3), (size_t) (i), a)

/* Get PTE flags accessors for each level. */
#define GET_PTL1_FLAGS_ARCH(ptl0, i) \
	get_pt_flags((pte_t *) (ptl0), (size_t) (i))
#define GET_PTL2_FLAGS_ARCH(ptl1, i) \
	get_pt_flags((pte_t *) (ptl1), (size_t) (i))
#define GET_PTL3_FLAGS_ARCH(ptl2, i) \
	get_pt_flags((pte_t *) (ptl2), (size_t) (i))
#define GET_FRAME_FLAGS_ARCH(ptl3, i) \
	get_pt_flags((pte_t *) (ptl3), (size_t) (i))

/* Set PTE flags accessors for each level. */
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x) \
	set_pt_flags((pte_t *) (ptl0), (size_t) (i), (x))
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x) \
	set_pt_flags((pte_t *) (ptl1), (size_t) (i), (x))
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x) \
	set_pt_flags((pte_t *) (ptl2), (size_t) (i), (x))
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pt_flags((pte_t *) (ptl3), (size_t) (i), (x))

/* Set PTE present bit accessors for each level. */
#define SET_PTL1_PRESENT_ARCH(ptl0, i) \
	set_pt_present((pte_t *) (ptl0), (size_t) (i))
#define SET_PTL2_PRESENT_ARCH(ptl1, i) \
	set_pt_present((pte_t *) (ptl1), (size_t) (i))
#define SET_PTL3_PRESENT_ARCH(ptl2, i) \
	set_pt_present((pte_t *) (ptl2), (size_t) (i))
#define SET_FRAME_PRESENT_ARCH(ptl3, i) \
	set_pt_present((pte_t *) (ptl3), (size_t) (i))

/* Macros for querying the last-level PTE entries. */
#define PTE_VALID_ARCH(p) \
	((p)->soft_valid != 0)
#define PTE_PRESENT_ARCH(p) \
	((p)->present != 0)
#define PTE_GET_FRAME_ARCH(p) \
	((((uintptr_t) (p)->addr_12_31) << 12) | \
	    ((uintptr_t) (p)->addr_32_51 << 32))
#define PTE_WRITABLE_ARCH(p) \
	((p)->writeable != 0)
#define PTE_EXECUTABLE_ARCH(p) \
	((p)->no_execute == 0)

#ifndef __ASSEMBLER__

#include <mm/mm.h>
#include <arch/interrupt.h>
#include <typedefs.h>

/* Page fault error codes. */

/** When bit on this position is 0, the page fault was caused by a not-present
 * page.
 */
#define PFERR_CODE_P  (1 << 0)

/** When bit on this position is 1, the page fault was caused by a write. */
#define PFERR_CODE_RW  (1 << 1)

/** When bit on this position is 1, the page fault was caused in user mode. */
#define PFERR_CODE_US  (1 << 2)

/** When bit on this position is 1, a reserved bit was set in page directory. */
#define PFERR_CODE_RSVD  (1 << 3)

/** When bit on this position os 1, the page fault was caused during instruction
 * fecth.
 */
#define PFERR_CODE_ID  (1 << 4)

/** Page Table Entry. */
typedef struct {
	unsigned int present : 1;
	unsigned int writeable : 1;
	unsigned int uaccessible : 1;
	unsigned int page_write_through : 1;
	unsigned int page_cache_disable : 1;
	unsigned int accessed : 1;
	unsigned int dirty : 1;
	unsigned int unused: 1;
	unsigned int global : 1;
	unsigned int soft_valid : 1;  /**< Valid content even if present bit is cleared. */
	unsigned int avl : 2;
	unsigned int addr_12_31 : 30;
	unsigned int addr_32_51 : 21;
	unsigned int no_execute : 1;
} __attribute__ ((packed)) pte_t;

NO_TRACE static inline unsigned int get_pt_flags(pte_t *pt, size_t i)
{
	pte_t *p = &pt[i];

	return ((!p->page_cache_disable) << PAGE_CACHEABLE_SHIFT |
	    (!p->present) << PAGE_PRESENT_SHIFT |
	    p->uaccessible << PAGE_USER_SHIFT |
	    1 << PAGE_READ_SHIFT |
	    p->writeable << PAGE_WRITE_SHIFT |
	    (!p->no_execute) << PAGE_EXEC_SHIFT |
	    p->global << PAGE_GLOBAL_SHIFT);
}

NO_TRACE static inline void set_pt_addr(pte_t *pt, size_t i, uintptr_t a)
{
	pte_t *p = &pt[i];

	p->addr_12_31 = (a >> 12) & UINT32_C(0xfffff);
	p->addr_32_51 = a >> 32;
}

NO_TRACE static inline void set_pt_flags(pte_t *pt, size_t i, int flags)
{
	pte_t *p = &pt[i];

	p->page_cache_disable = !(flags & PAGE_CACHEABLE);
	p->present = !(flags & PAGE_NOT_PRESENT);
	p->uaccessible = (flags & PAGE_USER) != 0;
	p->writeable = (flags & PAGE_WRITE) != 0;
	p->no_execute = (flags & PAGE_EXEC) == 0;
	p->global = (flags & PAGE_GLOBAL) != 0;

	/*
	 * Ensure that there is at least one bit set even if the present bit is cleared.
	 */
	p->soft_valid = 1;
}

NO_TRACE static inline void set_pt_present(pte_t *pt, size_t i)
{
	pte_t *p = &pt[i];

	p->present = 1;
}

extern void page_arch_init(void);
extern void page_fault(unsigned int, istate_t *);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
