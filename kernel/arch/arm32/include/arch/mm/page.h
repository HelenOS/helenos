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
#include <arch/cp15.h>
#include <trace.h>

#define PAGE_WIDTH	FRAME_WIDTH
#define PAGE_SIZE	FRAME_SIZE

#if (defined MACHINE_beagleboardxm) || (defined MACHINE_beaglebone)
#ifndef __ASSEMBLER__
#	define KA2PA(x)	((uintptr_t) (x))
#	define PA2KA(x)	((uintptr_t) (x))
#else
#	define KA2PA(x)	(x)
#	define PA2KA(x)	(x)
#endif
#else
#ifndef __ASSEMBLER__
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
#define PTL0_FRAMES_ARCH  4
#define PTL1_FRAMES_ARCH  1
#define PTL2_FRAMES_ARCH  1
#define PTL3_FRAMES_ARCH  1

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
	set_ptl0_addr((pte_t *) (ptl0))
#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
	set_ptl1_addr((pte_t*) (ptl0), i, a)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
	set_ptl3_addr((pte_t*) (ptl3), i, a)

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


#define pt_coherence(page) pt_coherence_m(page, 1)

#if defined(PROCESSOR_ARCH_armv6) | defined(PROCESSOR_ARCH_armv7_a)
#include "page_armv6.h"
#elif defined(PROCESSOR_ARCH_armv4) | defined(PROCESSOR_ARCH_armv5)
#include "page_armv4.h"
#else
#error "Unsupported architecture"
#endif

/** Sets the address of level 0 page table.
 *
 * @param pt Pointer to the page table to set.
 *
 * Page tables are always in cacheable memory.
 * Make sure the memory type is correct, and in sync with:
 * init_boot_pt (boot/arch/arm32/src/mm.c)
 * init_ptl0_section (boot/arch/arm32/src/mm.c)
 * set_pt_level1_flags (kernel/arch/arm32/include/arch/mm/page_armv6.h)
 */
NO_TRACE static inline void set_ptl0_addr(pte_t *pt)
{
	uint32_t val = (uint32_t)pt & TTBR_ADDR_MASK;
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	// FIXME: TTBR_RGN_WBWA_CACHE is unpredictable on ARMv6
	val |= TTBR_RGN_WBWA_CACHE | TTBR_C_FLAG;
#endif
	TTBR0_write(val);
}

NO_TRACE static inline void set_ptl1_addr(pte_t *pt, size_t i, uintptr_t address)
{
	pt[i].l0.coarse_table_addr = address >> 10;
	pt_coherence(&pt[i].l0);
}

NO_TRACE static inline void set_ptl3_addr(pte_t *pt, size_t i, uintptr_t address)
{
	pt[i].l1.frame_base_addr = address >> 12;
	pt_coherence(&pt[i].l1);
}

#endif

/** @}
 */
