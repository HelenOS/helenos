/*
 * Copyright (c) 2016 Martin Decky
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

/** @addtogroup kernel_riscv64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_PAGE_H_
#define KERN_riscv64_PAGE_H_

#include <arch/mm/frame.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#ifndef __ASSEMBLER__
#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffff800000000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT64_C(0xffff800000000000))
#else
#define KA2PA(x)  ((x) - 0xffff800000000000)
#define PA2KA(x)  ((x) + 0xffff800000000000)
#endif

/*
 * Implementation of 4-level page table interface.
 *
 * Page table layout:
 * - 64-bit virtual addressess
 *   (the virtual address space is 2^48 bytes in size
 *   with a hole in the middle)
 * - Offset is 12 bits => pages are 4 KiB long
 * - PTL0 has 512 entries (9 bits)
 * - PTL1 has 512 entries (9 bits)
 * - PTL2 has 512 entries (9 bits)
 * - PLT3 has 512 entries (9 bits)
 */

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

/* Macros calculating indices into page tables on each level. */
#define PTL0_INDEX_ARCH(vaddr)  (((vaddr) >> 39) & 0x1ff)
#define PTL1_INDEX_ARCH(vaddr)  (((vaddr) >> 30) & 0x1ff)
#define PTL2_INDEX_ARCH(vaddr)  (((vaddr) >> 21) & 0x1ff)
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> 12) & 0x1ff)

/* Flags mask for non-leaf page table entries */
#define NON_LEAF_MASK  (~(PAGE_READ | PAGE_WRITE | PAGE_EXEC))

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
	(((pte_t *) (ptl0))[(i)].pfn << 12)

#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
	(((pte_t *) (ptl1))[(i)].pfn << 12)

#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
	(((pte_t *) (ptl2))[(i)].pfn << 12)

#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
	(((pte_t *) (ptl3))[(i)].pfn << 12)

/* Set PTE address accessors for each level. */
#define SET_PTL0_ADDRESS_ARCH(ptl0) \
	(write_satp((uintptr_t) (ptl0)))

#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
	(((pte_t *) (ptl0))[(i)].pfn = (a) >> 12)

#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a) \
	(((pte_t *) (ptl1))[(i)].pfn = (a) >> 12)

#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a) \
	(((pte_t *) (ptl2))[(i)].pfn = (a) >> 12)

#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
	(((pte_t *) (ptl3))[(i)].pfn = (a) >> 12)

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
#define SET_PTL1_FLAGS_ARCH(ptl0, i, flags) \
	set_pt_flags((pte_t *) (ptl0), (size_t) (i), ((flags) & NON_LEAF_MASK))

#define SET_PTL2_FLAGS_ARCH(ptl1, i, flags) \
	set_pt_flags((pte_t *) (ptl1), (size_t) (i), ((flags) & NON_LEAF_MASK))

#define SET_PTL3_FLAGS_ARCH(ptl2, i, flags) \
	set_pt_flags((pte_t *) (ptl2), (size_t) (i), ((flags) & NON_LEAF_MASK))

#define SET_FRAME_FLAGS_ARCH(ptl3, i, flags) \
	set_pt_flags((pte_t *) (ptl3), (size_t) (i), (flags))

/* Set PTE present accessors for each level. */
#define SET_PTL1_PRESENT_ARCH(ptl0, i) \
	set_pt_present((pte_t *) (ptl0), (size_t) (i))

#define SET_PTL2_PRESENT_ARCH(ptl1, i) \
	set_pt_present((pte_t *) (ptl1), (size_t) (i))

#define SET_PTL3_PRESENT_ARCH(ptl2, i) \
	set_pt_present((pte_t *) (ptl2), (size_t) (i))

#define SET_FRAME_PRESENT_ARCH(ptl3, i) \
	set_pt_present((pte_t *) (ptl3), (size_t) (i))

/* Macros for querying the last-level PTEs. */
#define PTE_VALID_ARCH(pte)       ((pte)->valid != 0)
#define PTE_PRESENT_ARCH(pte)     ((pte)->valid != 0)
#define PTE_GET_FRAME_ARCH(pte)   ((uintptr_t) (pte)->pfn << 12)
#define PTE_WRITABLE_ARCH(pte)    ((pte)->writable != 0)
#define PTE_EXECUTABLE_ARCH(pte)  ((pte)->executable != 0)

#ifndef __ASSEMBLER__

#include <mm/mm.h>
#include <arch/interrupt.h>
#include <stdint.h>

/** Page Table Entry. */
typedef struct {
	unsigned long valid : 1;       /**< Valid bit. */
	unsigned long readable : 1;    /**< Readable bit. */
	unsigned long writable : 1;    /**< Writable bit. */
	unsigned long executable : 1;  /**< Executable bit. */
	unsigned long user : 1;        /**< User mode accessible bit. */
	unsigned long global : 1;      /**< Global mapping bit. */
	unsigned long accessed : 1;    /**< Accessed bit. */
	unsigned long dirty : 1;       /**< Dirty bit. */
	unsigned long reserved : 2;    /**< Reserved bits. */
	unsigned long pfn : 54;        /**< Physical frame number. */
} pte_t;

_NO_TRACE static inline unsigned int get_pt_flags(pte_t *pt, size_t i)
{
	pte_t *entry = &pt[i];

	return (((!entry->valid) << PAGE_PRESENT_SHIFT) |
	    (entry->user << PAGE_USER_SHIFT) |
	    (entry->readable << PAGE_READ_SHIFT) |
	    (entry->writable << PAGE_WRITE_SHIFT) |
	    (entry->executable << PAGE_EXEC_SHIFT) |
	    (entry->global << PAGE_GLOBAL_SHIFT));
}

_NO_TRACE static inline void set_pt_flags(pte_t *pt, size_t i, int flags)
{
	pte_t *entry = &pt[i];

	entry->valid = !(flags & PAGE_NOT_PRESENT);
	entry->readable = (flags & PAGE_READ) != 0;
	entry->writable = (flags & PAGE_WRITE) != 0;
	entry->executable = (flags & PAGE_EXEC) != 0;
	entry->user = (flags & PAGE_USER) != 0;
	entry->global = (flags & PAGE_GLOBAL) != 0;
	entry->accessed = 1;
	entry->dirty = 1;
}

_NO_TRACE static inline void set_pt_present(pte_t *pt, size_t i)
{
	pte_t *entry = &pt[i];

	entry->valid = 1;
}

extern void page_arch_init(void);
extern void page_fault(unsigned int, istate_t *);
extern void write_satp(uintptr_t);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
