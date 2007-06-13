/*
 * Copyright (c) 2007 Pavel Jancik
 * Copyright (c) 2007 Michal Kebrt
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


/** @addtogroup arm32boot
 * @{
 */
/** @file 
 *  @brief Memory management used while booting the kernel.
 *
 *  So called "section" paging is used while booting the kernel. The term
 *  "section" comes from the ARM architecture specification and stands for the
 *  following: one-level paging, 1MB sized pages, 4096 entries in the page
 *  table.
 */ 


#ifndef BOOT_arm32__MM_H
#define BOOT_arm32__MM_H


#ifndef __ASM__
#include "types.h"
#endif


/** Frame width. */
#define FRAME_WIDTH			20

/** Frame size. */
#define FRAME_SIZE			(1 << FRAME_WIDTH)

/** Page size in 2-level paging which is switched on later after the kernel
 * initialization.
 */
#define KERNEL_PAGE_SIZE		(1 << 12)


#ifndef __ASM__
/** Converts kernel address to physical address. */
#	define KA2PA(x)			(((uintptr_t) (x)) - 0x80000000)
/** Converts physical address to kernel address. */
#	define PA2KA(x)			(((uintptr_t) (x)) + 0x80000000)
#else
#	define KA2PA(x)			((x) - 0x80000000)
#	define PA2KA(x)			((x) + 0x80000000)
#endif


/** Number of entries in PTL0. */
#define PTL0_ENTRIES			(1 << 12)	/* 4096 */

/** Size of an entry in PTL0. */
#define PTL0_ENTRY_SIZE			4

/** Returns number of frame the address belongs to. */
#define ADDR2PFN(addr)			(((uintptr_t) (addr)) >> FRAME_WIDTH)

/** Describes "section" page table entry (one-level paging with 1MB sized pages). */  
#define PTE_DESCRIPTOR_SECTION		0x2

/** Page table access rights: user - no access, kernel - read/write. */
#define PTE_AP_USER_NO_KERNEL_RW	0x1


#ifndef __ASM__


/** Page table level 0 entry - "section" format is used (one-level paging, 1MB
 * sized pages). Used only while booting the kernel. 
 */
typedef struct {
	unsigned descriptor_type : 2;
	unsigned bufferable : 1;
	unsigned cacheable : 1; 
   	unsigned impl_specific : 1;
	unsigned domain : 4;
	unsigned should_be_zero_1 : 1;
	unsigned access_permission : 2; 	
	unsigned should_be_zero_2 : 8;
	unsigned section_base_addr : 12;
} __attribute__ ((packed)) pte_level0_section_t;


/** Page table that holds 1:1 virtual to physical mapping used while booting the
 * kernel.
 */ 
extern pte_level0_section_t page_table[PTL0_ENTRIES];

extern void mmu_start(void);


/** Enables paging. */
static inline void enable_paging()
{
	/* c3 - each two bits controls access to the one of domains (16)
	 *      0b01 - behave as a client (user) of a domain
	 */
	asm volatile (
		/* behave as a client of domains */
		"ldr r0, =0x55555555\n"
		"mcr p15, 0, r0, c3, c0, 0\n" 

		/* current settings */
		"mrc p15, 0, r0, c1, c0, 0\n"

		/* mask to enable paging */
		"ldr r1, =0x00000001\n"
		"orr r0, r0, r1\n"

		/* store settings */
		"mcr p15, 0, r0, c1, c0, 0\n"
		:
		:
		: "r0", "r1"
	);
}


/** Sets the address of level 0 page table to CP15 register 2.
 *
 * @param pt Address of a page table to set.
 */   
static inline void set_ptl0_address(pte_level0_section_t* pt)
{
	asm volatile (
		"mcr p15, 0, %0, c2, c0, 0\n"
		:
		: "r" (pt)
	);
}


#endif
 
#endif

/** @}
 */
 
