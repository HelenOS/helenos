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

#ifndef KERN_ppc32_PAGE_H_
#define KERN_ppc32_PAGE_H_

#include <arch/mm/frame.h>
#include <trace.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#ifndef __ASSEMBLER__
#define KA2PA(x)  (((uintptr_t) (x)) - 0x80000000)
#define PA2KA(x)  (((uintptr_t) (x)) + 0x80000000)
#else
#define KA2PA(x)  ((x) - 0x80000000)
#define PA2KA(x)  ((x) + 0x80000000)
#endif

/*
 * Implementation of generic 4-level page table interface,
 * the hardware Page Hash Table is used as cache.
 *
 * Page table layout:
 * - 32-bit virtual addressess
 * - Offset is 12 bits => pages are 4K long
 * - PTL0 has 1024 entries (10 bits)
 * - PTL1 is not used
 * - PTL2 is not used
 * - PLT3 has 1024 entries (10 bits)
 */

/* Number of entries in each level. */
#define PTL0_ENTRIES_ARCH  1024
#define PTL1_ENTRIES_ARCH  0
#define PTL2_ENTRIES_ARCH  0
#define PTL3_ENTRIES_ARCH  1024

/* Page table sizes for each level. */
#define PTL0_FRAMES_ARCH  1
#define PTL1_FRAMES_ARCH  1
#define PTL2_FRAMES_ARCH  1
#define PTL3_FRAMES_ARCH  1

/* Macros calculating indices into page tables on each level. */
#define PTL0_INDEX_ARCH(vaddr)  (((vaddr) >> 22) & 0x3ff)
#define PTL1_INDEX_ARCH(vaddr)  0
#define PTL2_INDEX_ARCH(vaddr)  0
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> 12) & 0x3ff)

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
	(((pte_t *) (ptl0))[(i)].pfn << 12)

#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
	(ptl1)

#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
	(ptl2)

#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
	(((pte_t *) (ptl3))[(i)].pfn << 12)

/* Set PTE address accessors for each level. */
#define SET_PTL0_ADDRESS_ARCH(ptl0)

#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
	(((pte_t *) (ptl0))[(i)].pfn = (a) >> 12)

#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)

#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
	(((pte_t *) (ptl3))[(i)].pfn = (a) >> 12)

/* Get PTE flags accessors for each level. */
#define GET_PTL1_FLAGS_ARCH(ptl0, i) \
	get_pt_flags((pte_t *) (ptl0), (size_t) (i))

#define GET_PTL2_FLAGS_ARCH(ptl1, i) \
	PAGE_PRESENT

#define GET_PTL3_FLAGS_ARCH(ptl2, i) \
	PAGE_PRESENT

#define GET_FRAME_FLAGS_ARCH(ptl3, i) \
	get_pt_flags((pte_t *) (ptl3), (size_t) (i))

/* Set PTE flags accessors for each level. */
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x) \
	set_pt_flags((pte_t *) (ptl0), (size_t) (i), (x))

#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)

#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pt_flags((pte_t *) (ptl3), (size_t) (i), (x))

/* Set PTE present accessors for each level. */
#define SET_PTL1_PRESENT_ARCH(ptl0, i) \
	set_pt_present((pte_t *) (ptl0), (size_t) (i))

#define SET_PTL2_PRESENT_ARCH(ptl1, i)
#define SET_PTL3_PRESENT_ARCH(ptl2, i)

#define SET_FRAME_PRESENT_ARCH(ptl3, i) \
	set_pt_present((pte_t *) (ptl3), (size_t) (i))

/* Macros for querying the last-level PTEs. */
#define PTE_VALID_ARCH(pte)       ((pte)->valid != 0)
#define PTE_PRESENT_ARCH(pte)     ((pte)->present != 0)
#define PTE_GET_FRAME_ARCH(pte)   ((pte)->pfn << 12)
#define PTE_WRITABLE_ARCH(pte)    1
#define PTE_EXECUTABLE_ARCH(pte)  1

#ifndef __ASSEMBLER__

#include <mm/mm.h>
#include <arch/interrupt.h>

/** Page Table Entry. */
typedef struct {
	unsigned int present : 1;             /**< Present bit. */
	unsigned int page_write_through : 1;  /**< Write thought caching. */
	unsigned int page_cache_disable : 1;  /**< No caching. */
	unsigned int accessed : 1;            /**< Accessed bit. */
	unsigned int global : 1;              /**< Global bit. */
	unsigned int valid : 1;               /**< Valid content even if not present. */
	unsigned int pfn : 20;                /**< Physical frame number. */
} pte_t;

NO_TRACE static inline unsigned int get_pt_flags(pte_t *pt, size_t i)
{
	pte_t *entry = &pt[i];

	return (((!entry->page_cache_disable) << PAGE_CACHEABLE_SHIFT) |
	    ((!entry->present) << PAGE_PRESENT_SHIFT) |
	    (1 << PAGE_USER_SHIFT) |
	    (1 << PAGE_READ_SHIFT) |
	    (1 << PAGE_WRITE_SHIFT) |
	    (1 << PAGE_EXEC_SHIFT) |
	    (entry->global << PAGE_GLOBAL_SHIFT));
}

NO_TRACE static inline void set_pt_flags(pte_t *pt, size_t i, int flags)
{
	pte_t *entry = &pt[i];

	entry->page_cache_disable = !(flags & PAGE_CACHEABLE);
	entry->present = !(flags & PAGE_NOT_PRESENT);
	entry->global = (flags & PAGE_GLOBAL) != 0;
	entry->valid = 1;
}

NO_TRACE static inline void set_pt_present(pte_t *pt, size_t i)
{
	pte_t *entry = &pt[i];

	entry->present = 1;
}

extern void page_arch_init(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
