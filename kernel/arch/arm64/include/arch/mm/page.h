/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief Paging related declarations.
 */

#ifndef KERN_arm64_PAGE_H_
#define KERN_arm64_PAGE_H_

#include <arch/mm/frame.h>
#include <mm/mm.h>
#include <trace.h>

#ifndef __ASSEMBLER__
#include <typedefs.h>
#endif /* __ASSEMBLER__ */

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#ifndef __ASSEMBLER__

extern uintptr_t physmem_base;

#define KA2PA(x) \
	(((uintptr_t) (x)) - UINT64_C(0xffffffff00000000) + physmem_base)
#define PA2KA(x) \
	(((uintptr_t) (x)) + UINT64_C(0xffffffff00000000) - physmem_base)

#endif /* __ASSEMBLER__ */

/** Log2 size of each translation table entry. */
#define PTL_ENTRY_SIZE_SHIFT  3

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

/* Starting bit of virtual address portion translated in each level. */
#define PTL0_VA_SHIFT  39
#define PTL1_VA_SHIFT  30
#define PTL2_VA_SHIFT  21
#define PTL3_VA_SHIFT  12

/* Size mask of virtual address portion translated in each level. */
#define PTL0_VA_MASK  0x1ff
#define PTL1_VA_MASK  0x1ff
#define PTL2_VA_MASK  0x1ff
#define PTL3_VA_MASK  0x1ff

/* Macros calculating indices into page tables for each level. */
#define PTL0_INDEX_ARCH(vaddr)  (((vaddr) >> PTL0_VA_SHIFT) & PTL0_VA_MASK)
#define PTL1_INDEX_ARCH(vaddr)  (((vaddr) >> PTL1_VA_SHIFT) & PTL1_VA_MASK)
#define PTL2_INDEX_ARCH(vaddr)  (((vaddr) >> PTL2_VA_SHIFT) & PTL2_VA_MASK)
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> PTL3_VA_SHIFT) & PTL3_VA_MASK)

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
	((pte_t *) (((uintptr_t) ((pte_t *) (ptl0))[(i)].output_address) << 12))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
	((pte_t *) (((uintptr_t) ((pte_t *) (ptl1))[(i)].output_address) << 12))
#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
	((pte_t *) (((uintptr_t) ((pte_t *) (ptl2))[(i)].output_address) << 12))
#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
	(((uintptr_t) ((pte_t *) (ptl3))[(i)].output_address) << 12)

/*
 * Set PTE address accessors for each level. Setting of the level 0 table is
 * ignored because it must be done only by calling as_install_arch() which also
 * changes ASID.
 */
#define SET_PTL0_ADDRESS_ARCH(ptl0)
#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
	(((pte_t *) (ptl0))[(i)].output_address = (a) >> 12)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a) \
	(((pte_t *) (ptl1))[(i)].output_address = (a) >> 12)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a) \
	(((pte_t *) (ptl2))[(i)].output_address = (a) >> 12)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
	(((pte_t *) (ptl3))[(i)].output_address = (a) >> 12)

/* Get PTE flags accessors for each level. */
#define GET_PTL1_FLAGS_ARCH(ptl0, i) \
	get_pt_level012_flags((pte_t *) (ptl0), (size_t) (i))
#define GET_PTL2_FLAGS_ARCH(ptl1, i) \
	get_pt_level012_flags((pte_t *) (ptl1), (size_t) (i))
#define GET_PTL3_FLAGS_ARCH(ptl2, i) \
	get_pt_level012_flags((pte_t *) (ptl2), (size_t) (i))
#define GET_FRAME_FLAGS_ARCH(ptl3, i) \
	get_pt_level3_flags((pte_t *) (ptl3), (size_t) (i))

/* Set PTE flags accessors for each level. */
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x) \
	set_pt_level012_flags((pte_t *) (ptl0), (size_t) (i), (x))
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x) \
	set_pt_level012_flags((pte_t *) (ptl1), (size_t) (i), (x))
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x) \
	set_pt_level012_flags((pte_t *) (ptl2), (size_t) (i), (x))
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pt_level3_flags((pte_t *) (ptl3), (size_t) (i), (x))

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
#define PTE_VALID_ARCH(pte) \
	(((pte_t *) (pte))->valid != 0)
#define PTE_PRESENT_ARCH(pte) \
	(((pte_t *) (pte))->valid != 0)
#define PTE_GET_FRAME_ARCH(pte) \
	(((uintptr_t) ((pte_t *) (pte))->output_address) << FRAME_WIDTH)
#define PTE_WRITABLE_ARCH(pte) \
	get_pt_writable((pte_t *) (pte))
#define PTE_EXECUTABLE_ARCH(pte) \
	get_pt_executable((pte_t *) (pte))

/* Level 3 access permissions. */

/** Data access permission. User mode: no access, privileged mode: read/write.
 */
#define PTE_AP_USER_NO_KERNEL_FULL  0

/** Data access permission. User mode: read/write, privileged mode: read/write.
 */
#define PTE_AP_USER_FULL_KERNEL_FULL  1

/** Data access permission. User mode: no access, privileged mode: read only. */
#define PTE_AP_USER_NO_KERNEL_LIMITED  2

/** Data access permission. User mode: read only, privileged mode: read only. */
#define PTE_AP_USER_LIMITED_KERNEL_LIMITED  3

/*
 * Memory types. MAIR_EL1 index 0 is unused, which assures that if a page
 * table entry is non-null then it is valid (PTE_VALID_ARCH() returns true).
 */

/** Write-Back Cacheable Normal memory, Inner shareable, Read-write cache
 * allocation. Defined in MAIR_EL1 index 1.
 */
#define MAIR_EL1_NORMAL_MEMORY_ATTR  0xff
#define MAIR_EL1_NORMAL_MEMORY_INDEX  1

/** Device-nGnRE memory (Device non-Gathering, non-Reordering, Early Write
 * Acknowledgement). Equivalent to the Device memory type in earlier versions of
 * the architecture. Defined in MAIR_EL1 index 2.
 */
#define MAIR_EL1_DEVICE_MEMORY_ATTR  0x04
#define MAIR_EL1_DEVICE_MEMORY_INDEX  2

/** Bit width of one memory attribute field in MAIR_EL1. */
#define MAIR_EL1_ATTR_SHIFT  8

/* Level 0, 1, 2 descriptor types. */

/** Block descriptor (valid in level 0, 1, 2 page translation tables). */
#define PTE_L012_TYPE_BLOCK  0

/** Next-table descriptor (valid in level 0, 1, 2 page translation tables). */
#define PTE_L012_TYPE_TABLE  1

/* Level 3 descriptor types. */

/** Page descriptor (valid in level 3 page translation tables). */
#define PTE_L3_TYPE_PAGE  1

/** HelenOS descriptor type. Table for level 0, 1, 2 page translation tables,
 * page for level 3 tables. Block descriptors are not used by HelenOS during
 * normal processing.
 */
#define PTE_L0123_TYPE_HELENOS  1

/* Page table entry access macros. */

/** Shift to access the next-level table address in a page table entry. */
#define PTE_NEXT_LEVEL_ADDRESS_SHIFT  12

/** Shift to access the resulting address in a page table entry. */
#define PTE_OUTPUT_ADDRESS_SHIFT  12

/** Shift to access the access bit in a page table entry. */
#define PTE_ACCESS_SHIFT  10

/** Shift to access the attr_index field in a page table entry. */
#define PTE_ATTR_INDEX_SHIFT  2

/** Shift to access the type bit in a page table entry. */
#define PTE_TYPE_SHIFT  1

/** Shift to access the present bit in a page table entry. */
#define PTE_PRESENT_SHIFT  0

/** The present bit in a page table entry. */
#define PTE_PRESENT_FLAG  (1 << PTE_PRESENT_SHIFT)

#ifndef __ASSEMBLER__

#include <arch/interrupt.h>

/** Page Table Entry.
 *
 * HelenOS model:
 * * Level 0, 1, 2 translation tables hold next-level table descriptors. Block
 *   descriptors are not used during normal processing.
 * * Level 3 tables store 4kB page descriptors.
 */
typedef struct {
	/* Common bits. */
	/** Flag indicating entry contains valid data and can be used for page
	 * translation.
	 *
	 * Note: The flag is called `valid' in the official ARM terminology
	 * but it has the `present' (valid+active) sense in HelenOS.
	 */
	unsigned valid : 1;
	unsigned type : 1;

	/* Lower block and page attributes. */
	unsigned attr_index : 3;
	unsigned non_secure : 1;
	unsigned access_permission : 2;
	unsigned shareability : 2;
	unsigned access : 1;
	unsigned not_global : 1;

	/* Common output address. */
	uint64_t output_address : 36;

	unsigned : 4;

	/* Upper block and page attributes. */
	unsigned contiguous : 1;
	unsigned privileged_execute_never : 1;
	unsigned unprivileged_execute_never : 1;

	unsigned : 4;

	/* Next-level table attributes. */
	unsigned privileged_execute_never_table : 1;
	unsigned unprivileged_execute_never_table : 1;
	unsigned access_permission_table : 2;
	unsigned non_secure_table : 1;
} __attribute__((packed)) pte_t;

/** Returns level 0, 1, 2 page table entry flags.
 *
 * @param pt Level 0, 1, 2 page table.
 * @param i  Index of the entry to return.
 */
_NO_TRACE static inline unsigned int get_pt_level012_flags(pte_t *pt, size_t i)
{
	pte_t *p = &pt[i];

	return (1 << PAGE_CACHEABLE_SHIFT) |
	    (!p->valid << PAGE_PRESENT_SHIFT) | (1 << PAGE_USER_SHIFT) |
	    (1 << PAGE_READ_SHIFT) | (1 << PAGE_WRITE_SHIFT) |
	    (1 << PAGE_EXEC_SHIFT);
}

/** Returns level 3 page table entry flags.
 *
 * @param pt Level 3 page table.
 * @param i  Index of the entry to return.
 */
_NO_TRACE static inline unsigned int get_pt_level3_flags(pte_t *pt, size_t i)
{
	pte_t *p = &pt[i];

	int cacheable = (p->attr_index == MAIR_EL1_NORMAL_MEMORY_INDEX);
	int user = (p->access_permission == PTE_AP_USER_FULL_KERNEL_FULL ||
	    p->access_permission == PTE_AP_USER_LIMITED_KERNEL_LIMITED);
	int write = (p->access_permission == PTE_AP_USER_FULL_KERNEL_FULL ||
	    p->access_permission == PTE_AP_USER_NO_KERNEL_FULL);
	int exec = ((user && !p->unprivileged_execute_never) ||
	    (!user && !p->privileged_execute_never));

	return (cacheable << PAGE_CACHEABLE_SHIFT) |
	    (!p->valid << PAGE_PRESENT_SHIFT) | (user << PAGE_USER_SHIFT) |
	    (1 << PAGE_READ_SHIFT) | (write << PAGE_WRITE_SHIFT) |
	    (exec << PAGE_EXEC_SHIFT) | (!p->not_global << PAGE_GLOBAL_SHIFT);
}

/** Sets flags of level 0, 1, 2 page table entry.
 *
 * @param pt    Level 0, 1, 2 page table.
 * @param i     Index of the entry to be changed.
 * @param flags New flags.
 */
_NO_TRACE static inline void set_pt_level012_flags(pte_t *pt, size_t i,
    int flags)
{
	pte_t *p = &pt[i];

	p->valid = (flags & PAGE_PRESENT) != 0;
	p->type = PTE_L012_TYPE_TABLE;
}

/** Sets flags of level 3 page table entry.
 *
 * @param pt    Level 3 page table.
 * @param i     Index of the entry to be changed.
 * @param flags New flags.
 */
_NO_TRACE static inline void set_pt_level3_flags(pte_t *pt, size_t i,
    int flags)
{
	pte_t *p = &pt[i];

	if (flags & PAGE_CACHEABLE)
		p->attr_index = MAIR_EL1_NORMAL_MEMORY_INDEX;
	else
		p->attr_index = MAIR_EL1_DEVICE_MEMORY_INDEX;
	p->valid = (flags & PAGE_PRESENT) != 0;
	p->type = PTE_L3_TYPE_PAGE;

	/* Translate page permissions to access permissions. */
	if (flags & PAGE_USER) {
		if (flags & PAGE_WRITE)
			p->access_permission = PTE_AP_USER_FULL_KERNEL_FULL;
		else
			p->access_permission =
			    PTE_AP_USER_LIMITED_KERNEL_LIMITED;
	} else {
		if (flags & PAGE_WRITE)
			p->access_permission = PTE_AP_USER_NO_KERNEL_FULL;
		else
			p->access_permission = PTE_AP_USER_NO_KERNEL_LIMITED;
	}
	p->access = 1;
	p->unprivileged_execute_never = p->privileged_execute_never =
	    (flags & PAGE_EXEC) == 0;

	p->not_global = (flags & PAGE_GLOBAL) == 0;
}

/** Sets the present flag of page table entry.
 *
 * @param pt Level 0, 1, 2, 3 page table.
 * @param i  Index of the entry to be changed.
 */
_NO_TRACE static inline void set_pt_present(pte_t *pt, size_t i)
{
	pte_t *p = &pt[i];

	p->valid = 1;
}

/** Gets the executable flag of page table entry.
 *
 * @param pte Page table entry.
 */
_NO_TRACE static inline bool get_pt_executable(pte_t *pte)
{
	if (pte->access_permission == PTE_AP_USER_NO_KERNEL_FULL ||
	    pte->access_permission == PTE_AP_USER_NO_KERNEL_LIMITED)
		return pte->privileged_execute_never;
	else
		return pte->unprivileged_execute_never;
}

/** Gets the writable flag of page table entry.
 *
 * @param pte Page table entry.
 */
_NO_TRACE static inline bool get_pt_writable(pte_t *pte)
{
	return pte->access_permission == PTE_AP_USER_FULL_KERNEL_FULL ||
	    pte->access_permission == PTE_AP_USER_NO_KERNEL_FULL;
}

extern void page_arch_init(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
