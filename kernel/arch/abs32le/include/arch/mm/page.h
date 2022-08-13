/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_mm
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_PAGE_H_
#define KERN_abs32le_PAGE_H_

#include <arch/mm/frame.h>
#include <stdbool.h>
#include <trace.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#define KA2PA(x)  (((uintptr_t) (x)) - UINT32_C(0x80000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT32_C(0x80000000))

/*
 * This is an example of 2-level page tables (PTL1 and PTL2 are left out)
 * on top of the generic 4-level page table interface.
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

/* Macros calculating indices for each level. */
#define PTL0_INDEX_ARCH(vaddr)  (((vaddr) >> 22) & 0x3ffU)
#define PTL1_INDEX_ARCH(vaddr)  0
#define PTL2_INDEX_ARCH(vaddr)  0
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> 12) & 0x3ffU)

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
	((pte_t *) ((((pte_t *) (ptl0))[(i)].frame_address) << 12))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
	(ptl1)
#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
	(ptl2)
#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
	((uintptr_t) ((((pte_t *) (ptl3))[(i)].frame_address) << 12))

/* Set PTE address accessors for each level. */
#define SET_PTL0_ADDRESS_ARCH(ptl0)
#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
	(((pte_t *) (ptl0))[(i)].frame_address = (a) >> 12)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
	(((pte_t *) (ptl3))[(i)].frame_address = (a) >> 12)

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
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x)	\
	set_pt_flags((pte_t *) (ptl0), (size_t) (i), (x))
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pt_flags((pte_t *) (ptl3), (size_t) (i), (x))

/* Set PTE present bit accessors for each level. */
#define SET_PTL1_PRESENT_ARCH(ptl0, i)	\
	set_pt_present((pte_t *) (ptl0), (size_t) (i))
#define SET_PTL2_PRESENT_ARCH(ptl1, i)
#define SET_PTL3_PRESENT_ARCH(ptl2, i)
#define SET_FRAME_PRESENT_ARCH(ptl3, i) \
	set_pt_present((pte_t *) (ptl3), (size_t) (i))

/* Macros for querying the last level entries. */
#define PTE_VALID_ARCH(p) \
	((p)->soft_valid != 0)
#define PTE_PRESENT_ARCH(p) \
	((p)->present != 0)
#define PTE_GET_FRAME_ARCH(p) \
	((p)->frame_address << FRAME_WIDTH)
#define PTE_WRITABLE_ARCH(p) \
	((p)->writeable != 0)
#define PTE_EXECUTABLE_ARCH(p)  1

#include <mm/mm.h>
#include <arch/interrupt.h>
#include <typedefs.h>

/** Page Table Entry. */
typedef struct {
	unsigned int present : 1;
	unsigned int writeable : 1;
	unsigned int uaccessible : 1;
	unsigned int page_write_through : 1;
	unsigned int page_cache_disable : 1;
	unsigned int accessed : 1;
	unsigned int dirty : 1;
	unsigned int pat : 1;
	unsigned int global : 1;

	/** Valid content even if the present bit is not set. */
	unsigned int soft_valid : 1;
	unsigned int avl : 2;
	unsigned int frame_address : 20;
} __attribute__((packed)) pte_t;

_NO_TRACE static inline unsigned int get_pt_flags(pte_t *pt, size_t i)
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];

	return (((unsigned int) (!p->page_cache_disable) << PAGE_CACHEABLE_SHIFT) |
	    ((unsigned int) (!p->present) << PAGE_PRESENT_SHIFT) |
	    ((unsigned int) p->uaccessible << PAGE_USER_SHIFT) |
	    (1 << PAGE_READ_SHIFT) |
	    ((unsigned int) p->writeable << PAGE_WRITE_SHIFT) |
	    (1 << PAGE_EXEC_SHIFT) |
	    ((unsigned int) p->global << PAGE_GLOBAL_SHIFT));
}

_NO_TRACE static inline void set_pt_flags(pte_t *pt, size_t i, int flags)
    WRITES(ARRAY_RANGE(pt, PTL0_ENTRIES_ARCH))
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];

	p->page_cache_disable = !(flags & PAGE_CACHEABLE);
	p->present = !(flags & PAGE_NOT_PRESENT);
	p->uaccessible = (flags & PAGE_USER) != 0;
	p->writeable = (flags & PAGE_WRITE) != 0;
	p->global = (flags & PAGE_GLOBAL) != 0;

	/*
	 * Ensure that there is at least one bit set even if the present bit is
	 * cleared.
	 */
	p->soft_valid = true;
}

_NO_TRACE static inline void set_pt_present(pte_t *pt, size_t i)
    WRITES(ARRAY_RANGE(pt, PTL0_ENTRIES_ARCH))
    REQUIRES_ARRAY_MUTABLE(pt, PTL0_ENTRIES_ARCH)
{
	pte_t *p = &pt[i];

	p->present = 1;
}

extern void page_arch_init(void);
extern void page_fault(unsigned int, istate_t *);

#endif

/** @}
 */
