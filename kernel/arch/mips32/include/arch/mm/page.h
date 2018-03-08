/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup mips32mm
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_PAGE_H_
#define KERN_mips32_PAGE_H_

#include <arch/mm/frame.h>
#include <trace.h>

#define PAGE_WIDTH	FRAME_WIDTH
#define PAGE_SIZE	FRAME_SIZE

#ifndef __ASSEMBLER__
#	define KA2PA(x)	(((uintptr_t) (x)) - 0x80000000)
#	define PA2KA(x)	(((uintptr_t) (x)) + 0x80000000)
#else
#	define KA2PA(x)	((x) - 0x80000000)
#	define PA2KA(x)	((x) + 0x80000000)
#endif

/*
 * Implementation of generic 4-level page table interface.
 *
 * Page table layout:
 * - 32-bit virtual addresses
 * - Offset is 14 bits => pages are 16K long
 * - PTE's use similar format as CP0 EntryLo[01] registers => PTE is therefore
 *   4 bytes long
 * - PTE's replace EntryLo v (valid) bit with p (present) bit
 * - PTE's use only one bit to distinguish between cacheable and uncacheable
 *   mappings
 * - PTE's define soft_valid field to ensure there is at least one 1 bit even if
 *   the p bit is cleared
 * - PTE's make use of CP0 EntryLo's two-bit reserved field for bit W (writable)
 *   and bit A (accessed)
 * - PTL0 has 64 entries (6 bits)
 * - PTL1 is not used
 * - PTL2 is not used
 * - PTL3 has 4096 entries (12 bits)
 */

/* Macros describing number of entries in each level. */
#define PTL0_ENTRIES_ARCH  64
#define PTL1_ENTRIES_ARCH  0
#define PTL2_ENTRIES_ARCH  0
#define PTL3_ENTRIES_ARCH  4096

/* Macros describing size of page tables in each level. */
#define PTL0_FRAMES_ARCH  1
#define PTL1_FRAMES_ARCH  1
#define PTL2_FRAMES_ARCH  1
#define PTL3_FRAMES_ARCH  1

/* Macros calculating entry indices for each level. */
#define PTL0_INDEX_ARCH(vaddr)  ((vaddr) >> 26)
#define PTL1_INDEX_ARCH(vaddr)  0
#define PTL2_INDEX_ARCH(vaddr)  0
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> 14) & 0xfff)

/* Set accessor for PTL0 address. */
#define SET_PTL0_ADDRESS_ARCH(ptl0)

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

/* Set PTE present bit accessors for each level. */
#define SET_PTL1_PRESENT_ARCH(ptl0, i) \
	set_pt_present((pte_t *) (ptl0), (size_t) (i))
#define SET_PTL2_PRESENT_ARCH(ptl1, i)
#define SET_PTL3_PRESENT_ARCH(ptl2, i)
#define SET_FRAME_PRESENT_ARCH(ptl3, i) \
	set_pt_present((pte_t *) (ptl3), (size_t) (i))

/* Last-level info macros. */
#define PTE_VALID_ARCH(pte)		((pte)->soft_valid != 0)
#define PTE_PRESENT_ARCH(pte)		((pte)->p != 0)
#define PTE_GET_FRAME_ARCH(pte)		((pte)->pfn << 12)
#define PTE_WRITABLE_ARCH(pte)		((pte)->w != 0)
#define PTE_EXECUTABLE_ARCH(pte)	1

#ifndef __ASSEMBLER__

#include <mm/mm.h>
#include <arch/exception.h>

/** Page Table Entry. */
typedef struct {
	unsigned g : 1;			/**< Global bit. */
	unsigned p : 1;			/**< Present bit. */
	unsigned d : 1;			/**< Dirty bit. */
	unsigned cacheable : 1;		/**< Cacheable bit. */
	unsigned : 1;			/**< Unused. */
	unsigned soft_valid : 1;	/**< Valid content even if not present. */
	unsigned pfn : 24;		/**< Physical frame number. */
	unsigned w : 1;			/**< Page writable bit. */
	unsigned a : 1;			/**< Accessed bit. */
} pte_t;


NO_TRACE static inline unsigned int get_pt_flags(pte_t *pt, size_t i)
{
	pte_t *p = &pt[i];

	return ((p->cacheable << PAGE_CACHEABLE_SHIFT) |
	    ((!p->p) << PAGE_PRESENT_SHIFT) |
	    (1 << PAGE_USER_SHIFT) |
	    (1 << PAGE_READ_SHIFT) |
	    ((p->w) << PAGE_WRITE_SHIFT) |
	    (1 << PAGE_EXEC_SHIFT) |
	    (p->g << PAGE_GLOBAL_SHIFT));
}

NO_TRACE static inline void set_pt_flags(pte_t *pt, size_t i, int flags)
{
	pte_t *p = &pt[i];

	p->cacheable = (flags & PAGE_CACHEABLE) != 0;
	p->p = !(flags & PAGE_NOT_PRESENT);
	p->g = (flags & PAGE_GLOBAL) != 0;
	p->w = (flags & PAGE_WRITE) != 0;

	/*
	 * Ensure that valid entries have at least one bit set.
	 */
	p->soft_valid = 1;
}

NO_TRACE static inline void set_pt_present(pte_t *pt, size_t i)
{
	pte_t *p = &pt[i];

	p->p = 1;
}

extern void page_arch_init(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
