/*
 * Copyright (c) 2003-2007 Jakub Jermar
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
 */

#ifndef KERN_arm32_PAGE_H_
#define KERN_arm32_PAGE_H_

#include <arch/mm/frame.h>

#define PAGE_WIDTH	FRAME_WIDTH
#define PAGE_SIZE	FRAME_SIZE

#define PAGE_COLOR_BITS	0			/* dummy */

#ifndef __ASM__
#	define KA2PA(x)	(((uintptr_t) (x)) - 0x80000000)
#	define PA2KA(x)	(((uintptr_t) (x)) + 0x80000000)
#else
#	define KA2PA(x)	((x) - 0x80000000)
#	define PA2KA(x)	((x) + 0x80000000)
#endif

#ifdef KERNEL

#define PTL0_ENTRIES_ARCH	0	/* TODO */
#define PTL1_ENTRIES_ARCH	0	/* TODO */
#define PTL2_ENTRIES_ARCH	0	/* TODO */
#define PTL3_ENTRIES_ARCH	0	/* TODO */

#define PTL0_INDEX_ARCH(vaddr)  0	/* TODO */ 
#define PTL1_INDEX_ARCH(vaddr)  0	/* TODO */
#define PTL2_INDEX_ARCH(vaddr)  0	/* TODO */
#define PTL3_INDEX_ARCH(vaddr)  0	/* TODO */

#define SET_PTL0_ADDRESS_ARCH(ptl0)

#define GET_PTL1_ADDRESS_ARCH(ptl0, i)		0	/* TODO */
#define GET_PTL2_ADDRESS_ARCH(ptl1, i)		0	/* TODO */
#define GET_PTL3_ADDRESS_ARCH(ptl2, i)		0	/* TODO */
#define GET_FRAME_ADDRESS_ARCH(ptl3, i)		0	/* TODO */

#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a)	/* TODO */
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)	/* TODO */
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)	/* TODO */
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a)	/* TODO */

#define GET_PTL1_FLAGS_ARCH(ptl0, i)		0	/* TODO */
#define GET_PTL2_FLAGS_ARCH(ptl1, i)		0	/* TODO */
#define GET_PTL3_FLAGS_ARCH(ptl2, i)		0	/* TODO */
#define GET_FRAME_FLAGS_ARCH(ptl3, i)		0	/* TODO */

#define SET_PTL1_FLAGS_ARCH(ptl0, i, x)		/* TODO */
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)		/* TODO */
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)		/* TODO */
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x)	/* TODO */

#define PTE_VALID_ARCH(pte)			0	/* TODO */
#define PTE_PRESENT_ARCH(pte)			0	/* TODO */
#define PTE_GET_FRAME_ARCH(pte)			0	/* TODO */
#define PTE_WRITABLE_ARCH(pte)			0	/* TODO */
#define PTE_EXECUTABLE_ARCH(pte)		0	/* TODO */

#ifndef __ASM__

#include <mm/mm.h>
#include <arch/exception.h>

static inline int get_pt_flags(pte_t *pt, index_t i)
{
	return 0;	/* TODO */
}

static inline void set_pt_flags(pte_t *pt, index_t i, int flags)
{
	/* TODO */
	return;
}

extern void page_arch_init(void);

#endif /* __ASM__ */

#endif /* KERNEL */

#endif

/** @}
 */
