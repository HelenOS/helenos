/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32mm
 * @{
 */
/** @file
 *  @brief Paging related declarations.
 */

#ifndef KERN_arm32_PAGE_H_
#define KERN_arm32_PAGE_H_

#include <arch/mm/frame.h>
#include <mm/mm.h>
#include <arch/exception.h>
#include <arch/barrier.h>
#include <trace.h>

#define PAGE_WIDTH	FRAME_WIDTH
#define PAGE_SIZE	FRAME_SIZE

#ifdef MACHINE_beagleboardxm
#ifndef __ASM__
#	define KA2PA(x)	((uintptr_t) (x))
#	define PA2KA(x)	((uintptr_t) (x))
#else
#	define KA2PA(x)	(x)
#	define PA2KA(x)	(x)
#endif
#else
#ifndef __ASM__
#	define KA2PA(x)	(((uintptr_t) (x)) - 0x80000000)
#	define PA2KA(x)	(((uintptr_t) (x)) + 0x80000000)
#else
#	define KA2PA(x)	((x) - 0x80000000)
#	define PA2KA(x)	((x) + 0x80000000)
#endif
#endif

/* Number of entries in each level. */
#define PTL0_ENTRIES_ARCH       (1 << 12)       /* 4096 */
#define PTL1_ENTRIES_ARCH       0
#define PTL2_ENTRIES_ARCH       0
/* coarse page tables used (256 * 4 = 1KB per page) */
#define PTL3_ENTRIES_ARCH       (1 << 8)        /* 256 */

/* Page table sizes for each level. */
#define PTL0_SIZE_ARCH          FOUR_FRAMES
#define PTL1_SIZE_ARCH          0
#define PTL2_SIZE_ARCH          0
#define PTL3_SIZE_ARCH          ONE_FRAME

/* Macros calculating indices into page tables for each level. */
#define PTL0_INDEX_ARCH(vaddr)  (((vaddr) >> 20) & 0xfff)
#define PTL1_INDEX_ARCH(vaddr)  0
#define PTL2_INDEX_ARCH(vaddr)  0
#define PTL3_INDEX_ARCH(vaddr)  (((vaddr) >> 12) & 0x0ff)

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
        ((pte_t *) ((((pte_t *)(ptl0))[(i)].l0).coarse_table_addr << 10))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
        (ptl1)
#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
        (ptl2)
#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
        ((uintptr_t) ((((pte_t *)(ptl3))[(i)].l1).frame_base_addr << 12))

/* Set PTE address accessors for each level. */
#define SET_PTL0_ADDRESS_ARCH(ptl0) \
        (set_ptl0_addr((pte_t *) (ptl0)))
#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
        (((pte_t *) (ptl0))[(i)].l0.coarse_table_addr = (a) >> 10)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
        (((pte_t *) (ptl3))[(i)].l1.frame_base_addr = (a) >> 12)

/* Get PTE flags accessors for each level. */
#define GET_PTL1_FLAGS_ARCH(ptl0, i) \
        get_pt_level0_flags((pte_t *) (ptl0), (size_t) (i))
#define GET_PTL2_FLAGS_ARCH(ptl1, i) \
        PAGE_PRESENT
#define GET_PTL3_FLAGS_ARCH(ptl2, i) \
        PAGE_PRESENT
#define GET_FRAME_FLAGS_ARCH(ptl3, i) \
        get_pt_level1_flags((pte_t *) (ptl3), (size_t) (i))

/* Set PTE flags accessors for each level. */
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x) \
        set_pt_level0_flags((pte_t *) (ptl0), (size_t) (i), (x))
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pt_level1_flags((pte_t *) (ptl3), (size_t) (i), (x))

/* Set PTE present bit accessors for each level. */
#define SET_PTL1_PRESENT_ARCH(ptl0, i) \
	set_pt_level0_present((pte_t *) (ptl0), (size_t) (i))
#define SET_PTL2_PRESENT_ARCH(ptl1, i)
#define SET_PTL3_PRESENT_ARCH(ptl2, i)
#define SET_FRAME_PRESENT_ARCH(ptl3, i) \
	set_pt_level1_present((pte_t *) (ptl3), (size_t) (i))

#if defined(PROCESSOR_armv7_a)
#include "page_armv7.h"
#elif defined(PROCESSOR_armv4) | defined(PROCESSOR_armv5)
#include "page_armv4.h"
#endif

#ifndef __ASM__
NO_TRACE static inline void set_pt_level0_present(pte_t *pt, size_t i)
{
	pte_level0_t *p = &pt[i].l0;

	p->should_be_zero = 0;
	write_barrier();
	p->descriptor_type = PTE_DESCRIPTOR_COARSE_TABLE;
}


NO_TRACE static inline void set_pt_level1_present(pte_t *pt, size_t i)
{
	pte_level1_t *p = &pt[i].l1;

	p->descriptor_type = PTE_DESCRIPTOR_SMALL_PAGE;
}

#endif /* __ASM__ */

#endif

/** @}
 */
