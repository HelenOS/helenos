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

/** @addtogroup riscv64mm
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_PAGE_H_
#define KERN_riscv64_PAGE_H_

#include <arch/mm/frame.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#ifndef __ASM__
	#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffff800000000000))
	#define PA2KA(x)  (((uintptr_t) (x)) + UINT64_C(0xffff800000000000))
#else
	#define KA2PA(x)  ((x) - 0xffff800000000000)
	#define PA2KA(x)  ((x) + 0xffff800000000000)
#endif

/*
 * Page table entry types.
 *
 * - PTE_TYPE_PTR:         pointer to next level PTE
 * - PTE_TYPE_PTR_GLOBAL:  pointer to next level PTE (global mapping)
 *
 * - PTE_TYPE_SRURX:       kernel read, user read execute
 * - PTE_TYPE_SRWURWX:     kernel read write, user read write execute
 * - PTE_TYPE_SRUR:        kernel read, user read
 * - PTE_TYPE_SRWURW:      kernel read write, user read write
 * - PTE_TYPE_SRXURX:      kernel read execute, user read execute
 * - PTE_TYPE_SRWXURWX:    kernel read write execute, user read write execute
 *
 * - PTE_TYPE_SR:          kernel read
 * - PTE_TYPE_SRW:         kernel read write
 * - PTE_TYPE_SRX:         kernel read execute
 * - PTE_TYPE_SRWX:        kernel read write execute
 *
 * - PTE_TYPE_SR_GLOBAL:   kernel read (global mapping)
 * - PTE_TYPE_SRW_GLOBAL:  kernel read write (global mapping)
 * - PTE_TYPE_SRX_GLOBAL:  kernel read execute (global mapping)
 * - PTE_TYPE_SRWX_GLOBAL: kernel read write execute (global mapping)
 */

#define PTE_TYPE_PTR          0
#define PTE_TYPE_PTR_GLOBAL   1

#define PTE_TYPE_SRURX        2
#define PTE_TYPE_SRWURWX      3
#define PTE_TYPE_SRUR         4
#define PTE_TYPE_SRWURW       5
#define PTE_TYPE_SRXURX       6
#define PTE_TYPE_SRWXURWX     7

#define PTE_TYPE_SR           8
#define PTE_TYPE_SRW          9
#define PTE_TYPE_SRX          10
#define PTE_TYPE_SRWX         11

#define PTE_TYPE_SR_GLOBAL    12
#define PTE_TYPE_SRW_GLOBAL   13
#define PTE_TYPE_SRX_GLOBAL   14
#define PTE_TYPE_SRWX_GLOBAL  15

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
#define SET_PTL0_ADDRESS_ARCH(ptl0)

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
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x) \
	set_pt_flags((pte_t *) (ptl0), (size_t) (i), (x))

#define SET_PTL2_FLAGS_ARCH(ptl1, i, x) \
	set_pt_flags((pte_t *) (ptl1), (size_t) (i), (x))

#define SET_PTL3_FLAGS_ARCH(ptl2, i, x) \
	set_pt_flags((pte_t *) (ptl2), (size_t) (i), (x))

#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pt_flags((pte_t *) (ptl3), (size_t) (i), (x))

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

#define PTE_WRITABLE_ARCH(pte) \
	(((pte)->type == PTE_TYPE_SRWURWX) || \
	((pte)->type == PTE_TYPE_SRWURW) || \
	((pte)->type == PTE_TYPE_SRWXURWX))

#define PTE_EXECUTABLE_ARCH(pte) \
	(((pte)->type == PTE_TYPE_SRURX) || \
	((pte)->type == PTE_TYPE_SRWURWX) || \
	((pte)->type == PTE_TYPE_SRXURX) || \
	((pte)->type == PTE_TYPE_SRWXURWX))

#ifndef __ASM__

#include <mm/mm.h>
#include <arch/interrupt.h>
#include <typedefs.h>

/** Page Table Entry. */
typedef struct {
	unsigned long valid : 1;       /**< Valid bit. */
	unsigned long type : 4;        /**< Entry type. */
	unsigned long referenced : 1;  /**< Refenced bit. */
	unsigned long dirty : 1;       /**< Dirty bit. */
	unsigned long reserved : 3;    /**< Reserved bits. */
	unsigned long pfn : 54;        /**< Physical frame number. */
} pte_t;

NO_TRACE static inline unsigned int get_pt_flags(pte_t *pt, size_t i)
{
	pte_t *entry = &pt[i];
	
	return (((!entry->valid) << PAGE_PRESENT_SHIFT) |
	    ((entry->type < PTE_TYPE_SR) << PAGE_USER_SHIFT) |
	    PAGE_READ |
	    (PTE_WRITABLE_ARCH(entry) << PAGE_WRITE_SHIFT) |
	    (PTE_EXECUTABLE_ARCH(entry) << PAGE_EXEC_SHIFT) |
	    ((entry->type >= PTE_TYPE_SR_GLOBAL) << PAGE_GLOBAL_SHIFT));
}

NO_TRACE static inline void set_pt_flags(pte_t *pt, size_t i, int flags)
{
	pte_t *entry = &pt[i];
	
	entry->valid = !(flags & PAGE_NOT_PRESENT);
	
	if ((flags & PAGE_WRITE) != 0) {
		if ((flags & PAGE_EXEC) != 0)
			entry->type = PTE_TYPE_SRWXURWX;
		else
			entry->type = PTE_TYPE_SRWURW;
	} else {
		if ((flags & PAGE_EXEC) != 0)
			entry->type = PTE_TYPE_SRXURX;
		else
			entry->type = PTE_TYPE_SRUR;
	}
}

NO_TRACE static inline void set_pt_present(pte_t *pt, size_t i)
{
	pte_t *entry = &pt[i];

	entry->valid = 1;
}

extern void page_arch_init(void);
extern void page_fault(unsigned int, istate_t *);

#endif /* __ASM__ */

#endif

/** @}
 */
