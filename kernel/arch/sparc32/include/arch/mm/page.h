/*
 * Copyright (c) 2010 Martin Decky
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup sparc32mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc32_PAGE_H_
#define KERN_sparc32_PAGE_H_

#ifndef __ASM__

#include <arch/mm/frame.h>
#include <trace.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#define FRAME_LOWPRIO  0

#define KA2PA(x)  (((uintptr_t) (x)) - UINT32_C(0x40000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT32_C(0x40000000))

#define PTE_ET_INVALID     0
#define PTE_ET_DESCRIPTOR  1
#define PTE_ET_ENTRY       2

#define PTE_ACC_USER_RO_KERNEL_RO    0
#define PTE_ACC_USER_RW_KERNEL_RW    1
#define PTE_ACC_USER_RX_KERNEL_RX    2
#define PTE_ACC_USER_RWX_KERNEL_RWX  3
#define PTE_ACC_USER_XO_KERNEL_XO    4
#define PTE_ACC_USER_RO_KERNEL_RW    5
#define PTE_ACC_USER_NO_KERNEL_RX    6
#define PTE_ACC_USER_NO_KERNEL_RWX   7

/* Number of entries in each level. */
#define PTL0_ENTRIES_ARCH  256
#define PTL1_ENTRIES_ARCH  0
#define PTL2_ENTRIES_ARCH  64
#define PTL3_ENTRIES_ARCH  64

/* Page table sizes for each level. */
#define PTL0_FRAMES_ARCH  1
#define PTL1_FRAMES_ARCH  1
#define PTL2_FRAMES_ARCH  1
#define PTL3_FRAMES_ARCH  1

/* Macros calculating indices for each level. */
#define PTL0_INDEX_ARCH(vaddr)  (((vaddr) >> 24) & 0xffU)
#define PTL1_INDEX_ARCH(vaddr)  0
#define PTL2_INDEX_ARCH(vaddr)  (((vaddr) >> 18) & 0x3fU)
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> 12) & 0x3fU)

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
	((pte_t *) ((((pte_t *) (ptl0))[(i)].frame_address) << 12))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
	KA2PA(ptl1)
#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
	((pte_t *) ((((pte_t *) (ptl2))[(i)].frame_address) << 12))
#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
	((uintptr_t) ((((pte_t *) (ptl3))[(i)].frame_address) << 12))

/* Set PTE address accessors for each level. */
#define SET_PTL0_ADDRESS_ARCH(ptl0)
#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
	(((pte_t *) (ptl0))[(i)].frame_address = (a) >> 12)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a) \
	(((pte_t *) (ptl2))[(i)].frame_address = (a) >> 12)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
	(((pte_t *) (ptl3))[(i)].frame_address = (a) >> 12)

/* Get PTE flags accessors for each level. */
#define GET_PTL1_FLAGS_ARCH(ptl0, i) \
	get_pt_flags((pte_t *) (ptl0), (size_t) (i))
#define GET_PTL2_FLAGS_ARCH(ptl1, i) \
	PAGE_PRESENT
#define GET_PTL3_FLAGS_ARCH(ptl2, i) \
	get_pt_flags((pte_t *) (ptl2), (size_t) (i))
#define GET_FRAME_FLAGS_ARCH(ptl3, i) \
	get_pt_flags((pte_t *) (ptl3), (size_t) (i))

/* Set PTE flags accessors for each level. */
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x)
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pte_flags((pte_t *) (ptl3), (size_t) (i), (x))

/* Set PTE present bit accessors for each level. */
#define SET_PTL1_PRESENT_ARCH(ptl0, i)	\
	set_ptd_present((pte_t *) (ptl0), (size_t) (i))
#define SET_PTL2_PRESENT_ARCH(ptl1, i)
#define SET_PTL3_PRESENT_ARCH(ptl2, i) \
	set_ptd_present((pte_t *) (ptl2), (size_t) (i))
#define SET_FRAME_PRESENT_ARCH(ptl3, i) \
	set_pte_present((pte_t *) (ptl3), (size_t) (i))

/* Macros for querying the last level entries. */
#define PTE_VALID_ARCH(p) \
	(*((uint32_t *) (p)) != 0)
#define PTE_PRESENT_ARCH(p) \
	((p)->et != 0)
#define PTE_GET_FRAME_ARCH(p) \
	((p)->frame_address << FRAME_WIDTH)
#define PTE_WRITABLE_ARCH(p) \
	pte_is_writeable(p)
#define PTE_EXECUTABLE_ARCH(p) \
	pte_is_executable(p)

#include <mm/mm.h>
#include <arch/interrupt.h>
#include <typedefs.h>

/** Page Table Descriptor. */
typedef struct {
	unsigned int table_pointer : 30;
	unsigned int et : 2;
} __attribute__((packed)) ptd_t;

/** Page Table Entry. */
typedef struct {
	unsigned int frame_address : 24;
	unsigned int cacheable : 1;
	unsigned int modified : 1;
	unsigned int referenced : 1;
	unsigned int acc : 3;
	unsigned int et : 2;
} __attribute__((packed)) pte_t;

NO_TRACE static inline void set_ptl0_addr(pte_t *pt)
{
}

NO_TRACE static inline bool pte_is_writeable(pte_t *pt)
{
	return ((pt->acc == PTE_ACC_USER_RW_KERNEL_RW) ||
	    (pt->acc == PTE_ACC_USER_RWX_KERNEL_RWX) ||
	    (pt->acc == PTE_ACC_USER_RO_KERNEL_RW) ||
	    (pt->acc == PTE_ACC_USER_NO_KERNEL_RWX));
}

NO_TRACE static inline bool pte_is_executable(pte_t *pt)
{
	return ((pt->acc != PTE_ACC_USER_RO_KERNEL_RO) &&
	    (pt->acc != PTE_ACC_USER_RW_KERNEL_RW) &&
	    (pt->acc != PTE_ACC_USER_RO_KERNEL_RW));
}

NO_TRACE static inline unsigned int get_pt_flags(pte_t *pt, size_t i)
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];
	
	bool notpresent = (p->et == 0);
	
	return ((p->cacheable << PAGE_CACHEABLE_SHIFT) |
	    (notpresent << PAGE_PRESENT_SHIFT) |
	    (((p->acc != PTE_ACC_USER_NO_KERNEL_RX) &&
	    (p->acc != PTE_ACC_USER_NO_KERNEL_RWX)) << PAGE_USER_SHIFT) |
	    (1 << PAGE_READ_SHIFT) |
	    (((p->acc == PTE_ACC_USER_RW_KERNEL_RW) ||
	    (p->acc == PTE_ACC_USER_RWX_KERNEL_RWX) ||
	    (p->acc == PTE_ACC_USER_RO_KERNEL_RW) ||
	    (p->acc == PTE_ACC_USER_NO_KERNEL_RWX)) << PAGE_WRITE_SHIFT) |
	    (((p->acc != PTE_ACC_USER_RO_KERNEL_RO) &&
	    (p->acc != PTE_ACC_USER_RW_KERNEL_RW) &&
	    (p->acc != PTE_ACC_USER_RO_KERNEL_RW)) << PAGE_EXEC_SHIFT) |
	    (1 << PAGE_GLOBAL_SHIFT));
}

NO_TRACE static inline void set_ptd_flags(pte_t *pt, size_t i, int flags)
    WRITES(ARRAY_RANGE(pt, PTL0_ENTRIES_ARCH))
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];
	
	p->et = (flags & PAGE_NOT_PRESENT) ?
	    PTE_ET_INVALID : PTE_ET_DESCRIPTOR;
}

NO_TRACE static inline void set_pte_flags(pte_t *pt, size_t i, int flags)
    WRITES(ARRAY_RANGE(pt, PTL0_ENTRIES_ARCH))
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];
	
	p->et = PTE_ET_ENTRY;
	p->acc = PTE_ACC_USER_NO_KERNEL_RWX;
	
	if (flags & PAGE_USER) {
		if (flags & PAGE_EXEC) {
			if (flags & PAGE_READ)
				p->acc = PTE_ACC_USER_RX_KERNEL_RX;
			if (flags & PAGE_WRITE)
				p->acc = PTE_ACC_USER_RWX_KERNEL_RWX;
		} else {
			if (flags & PAGE_READ)
				p->acc = PTE_ACC_USER_RO_KERNEL_RW;
			if (flags & PAGE_WRITE)
				p->acc = PTE_ACC_USER_RW_KERNEL_RW;
		}
	}
	
	if (flags & PAGE_NOT_PRESENT)
		p->et = PTE_ET_INVALID;
	
	p->cacheable = (flags & PAGE_CACHEABLE) != 0;
}

NO_TRACE static inline void set_ptd_present(pte_t *pt, size_t i)
    WRITES(ARRAY_RANGE(pt, PTL0_ENTRIES_ARCH))
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];
	
	p->et = PTE_ET_DESCRIPTOR;
}

NO_TRACE static inline void set_pte_present(pte_t *pt, size_t i)
    WRITES(ARRAY_RANGE(pt, PTL0_ENTRIES_ARCH))
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];
	
	p->et = PTE_ET_ENTRY;
}

extern void page_arch_init(void);
extern void page_fault(unsigned int, istate_t *);

#endif

#endif

/** @}
 */
