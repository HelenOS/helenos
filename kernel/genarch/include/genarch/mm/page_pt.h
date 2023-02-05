/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kernel_genarch_mm
 * @{
 */
/** @file
 */

/*
 * This is the generic 4-level page table interface.
 * Architectures that use hierarchical page tables
 * are supposed to implement *_ARCH macros.
 *
 */

#ifdef CONFIG_PAGE_PT

#ifndef KERN_PAGE_PT_H_
#define KERN_PAGE_PT_H_

#include <mm/as.h>
#include <mm/page.h>
#include <arch/mm/page.h>
#include <typedefs.h>

/*
 * Number of entries in each level.
 */
#define PTL0_ENTRIES  PTL0_ENTRIES_ARCH
#define PTL1_ENTRIES  PTL1_ENTRIES_ARCH
#define PTL2_ENTRIES  PTL2_ENTRIES_ARCH
#define PTL3_ENTRIES  PTL3_ENTRIES_ARCH

/* Table sizes in each level (in frames) */
#define PTL0_FRAMES  PTL0_FRAMES_ARCH
#define PTL1_FRAMES  PTL1_FRAMES_ARCH
#define PTL2_FRAMES  PTL2_FRAMES_ARCH
#define PTL3_FRAMES  PTL3_FRAMES_ARCH

/* Table sizes in each level (in bytes) */
#define PTL0_SIZE  FRAMES2SIZE(PTL0_FRAMES)
#define PTL1_SIZE  FRAMES2SIZE(PTL1_FRAMES)
#define PTL2_SIZE  FRAMES2SIZE(PTL2_FRAMES)
#define PTL3_SIZE  FRAMES2SIZE(PTL3_FRAMES)

/*
 * These macros process vaddr and extract those portions
 * of it that function as indices to respective page tables.
 *
 */
#define PTL0_INDEX(vaddr)  PTL0_INDEX_ARCH(vaddr)
#define PTL1_INDEX(vaddr)  PTL1_INDEX_ARCH(vaddr)
#define PTL2_INDEX(vaddr)  PTL2_INDEX_ARCH(vaddr)
#define PTL3_INDEX(vaddr)  PTL3_INDEX_ARCH(vaddr)

#define SET_PTL0_ADDRESS(ptl0)  SET_PTL0_ADDRESS_ARCH(ptl0)

/*
 * These macros traverse the 4-level tree of page tables,
 * each descending by one level.
 *
 */
#define GET_PTL1_ADDRESS(ptl0, i)   GET_PTL1_ADDRESS_ARCH(ptl0, i)
#define GET_PTL2_ADDRESS(ptl1, i)   GET_PTL2_ADDRESS_ARCH(ptl1, i)
#define GET_PTL3_ADDRESS(ptl2, i)   GET_PTL3_ADDRESS_ARCH(ptl2, i)
#define GET_FRAME_ADDRESS(ptl3, i)  GET_FRAME_ADDRESS_ARCH(ptl3, i)

/*
 * These macros are provided to change the shape of the 4-level tree of page
 * tables on respective level.
 *
 */
#define SET_PTL1_ADDRESS(ptl0, i, a)   SET_PTL1_ADDRESS_ARCH(ptl0, i, a)
#define SET_PTL2_ADDRESS(ptl1, i, a)   SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS(ptl2, i, a)   SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS(ptl3, i, a)  SET_FRAME_ADDRESS_ARCH(ptl3, i, a)

/*
 * These macros are provided to query various flags within the page tables.
 *
 */
#define GET_PTL1_FLAGS(ptl0, i)   GET_PTL1_FLAGS_ARCH(ptl0, i)
#define GET_PTL2_FLAGS(ptl1, i)   GET_PTL2_FLAGS_ARCH(ptl1, i)
#define GET_PTL3_FLAGS(ptl2, i)   GET_PTL3_FLAGS_ARCH(ptl2, i)
#define GET_FRAME_FLAGS(ptl3, i)  GET_FRAME_FLAGS_ARCH(ptl3, i)

/*
 * These macros are provided to set/clear various flags within the page tables.
 *
 */
#define SET_PTL1_FLAGS(ptl0, i, x)   SET_PTL1_FLAGS_ARCH(ptl0, i, x)
#define SET_PTL2_FLAGS(ptl1, i, x)   SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS(ptl2, i, x)   SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS(ptl3, i, x)  SET_FRAME_FLAGS_ARCH(ptl3, i, x)

/*
 * These macros are provided to set the present bit within the page tables.
 *
 */
#define SET_PTL1_PRESENT(ptl0, i)   SET_PTL1_PRESENT_ARCH(ptl0, i)
#define SET_PTL2_PRESENT(ptl1, i)   SET_PTL2_PRESENT_ARCH(ptl1, i)
#define SET_PTL3_PRESENT(ptl2, i)   SET_PTL3_PRESENT_ARCH(ptl2, i)
#define SET_FRAME_PRESENT(ptl3, i)  SET_FRAME_PRESENT_ARCH(ptl3, i)

/*
 * Macros for querying the last-level PTEs.
 *
 */
#define PTE_VALID(p)       PTE_VALID_ARCH((p))
#define PTE_PRESENT(p)     PTE_PRESENT_ARCH((p))
#define PTE_GET_FRAME(p)   PTE_GET_FRAME_ARCH((p))
#define PTE_READABLE(p)    1
#define PTE_WRITABLE(p)    PTE_WRITABLE_ARCH((p))
#define PTE_EXECUTABLE(p)  PTE_EXECUTABLE_ARCH((p))

extern const as_operations_t as_pt_operations;
extern const page_mapping_operations_t pt_mapping_operations;

extern void page_mapping_insert_pt(as_t *, uintptr_t, uintptr_t, unsigned int);
extern pte_t *page_mapping_find_pt(as_t *, uintptr_t, bool);

#endif

#endif

/** @}
 */
