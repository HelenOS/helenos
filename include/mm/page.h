/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __PAGE_H__
#define __PAGE_H__

#include <arch/types.h>
#include <typedefs.h>

#define PAGE_CACHEABLE_SHIFT		0
#define PAGE_NOT_CACHEABLE_SHIFT	PAGE_CACHEABLE_SHIFT
#define PAGE_PRESENT_SHIFT		1
#define PAGE_NOT_PRESENT_SHIFT		PAGE_PRESENT_SHIFT
#define PAGE_USER_SHIFT			2
#define PAGE_KERNEL_SHIFT		PAGE_USER_SHIFT
#define PAGE_READ_SHIFT			3
#define PAGE_WRITE_SHIFT		4
#define PAGE_EXEC_SHIFT			5

#define PAGE_NOT_CACHEABLE	(0<<PAGE_CACHEABLE_SHIFT)
#define PAGE_CACHEABLE		(1<<PAGE_CACHEABLE_SHIFT)

#define PAGE_PRESENT		(0<<PAGE_PRESENT_SHIFT)
#define PAGE_NOT_PRESENT	(1<<PAGE_PRESENT_SHIFT)

#define PAGE_USER		(1<<PAGE_USER_SHIFT)
#define PAGE_KERNEL		(0<<PAGE_USER_SHIFT)

#define PAGE_READ		(1<<PAGE_READ_SHIFT)
#define PAGE_WRITE		(1<<PAGE_WRITE_SHIFT)
#define PAGE_EXEC		(1<<PAGE_EXEC_SHIFT)

/*
 * This is the generic 4-level page table interface.
 * Architectures are supposed to implement *_ARCH macros.
 */

/*
 * These macros process vaddr and extract those portions
 * of it that function as indices to respective page tables.
 */
#define PTL0_INDEX(vaddr)		PTL0_INDEX_ARCH(vaddr)
#define PTL1_INDEX(vaddr)		PTL1_INDEX_ARCH(vaddr)
#define PTL2_INDEX(vaddr)		PTL2_INDEX_ARCH(vaddr)
#define PTL3_INDEX(vaddr)		PTL3_INDEX_ARCH(vaddr)

#define GET_PTL0_ADDRESS()		GET_PTL0_ADDRESS_ARCH()
#define SET_PTL0_ADDRESS(ptl0)		SET_PTL0_ADDRESS_ARCH(ptl0)

/*
 * These macros traverse the 4-level tree of page tables,
 * each descending by one level.
 */
#define GET_PTL1_ADDRESS(ptl0, i)	GET_PTL1_ADDRESS_ARCH(ptl0, i)
#define GET_PTL2_ADDRESS(ptl1, i)	GET_PTL2_ADDRESS_ARCH(ptl1, i)
#define GET_PTL3_ADDRESS(ptl2, i)	GET_PTL3_ADDRESS_ARCH(ptl2, i)
#define GET_FRAME_ADDRESS(ptl3, i)	GET_FRAME_ADDRESS_ARCH(ptl3, i)

/*
 * These macros are provided to change shape of the 4-level
 * tree of page tables on respective level.
 */
#define SET_PTL1_ADDRESS(ptl0, i, a)	SET_PTL1_ADDRESS_ARCH(ptl0, i, a)
#define SET_PTL2_ADDRESS(ptl1, i, a)	SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS(ptl2, i, a)	SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS(ptl3, i, a)	SET_FRAME_ADDRESS_ARCH(ptl3, i, a)

/*
 * These macros are provided to query various flags within the page tables.
 */
#define GET_PTL1_FLAGS(ptl0, i)		GET_PTL1_FLAGS_ARCH(ptl0, i)
#define GET_PTL2_FLAGS(ptl1, i)		GET_PTL2_FLAGS_ARCH(ptl1, i)
#define GET_PTL3_FLAGS(ptl2, i)		GET_PTL3_FLAGS_ARCH(ptl2, i)
#define GET_FRAME_FLAGS(ptl3, i)	GET_FRAME_FLAGS_ARCH(ptl3, i)

/*
 * These macros are provided to set/clear various flags within the page tables.
 */
#define SET_PTL1_FLAGS(ptl0, i, x)	SET_PTL1_FLAGS_ARCH(ptl0, i, x)
#define SET_PTL2_FLAGS(ptl1, i, x)	SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS(ptl2, i, x)	SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS(ptl3, i, x)	SET_FRAME_FLAGS_ARCH(ptl3, i, x)

extern void page_init(void);
extern void map_page_to_frame(__address page, __address frame, int flags, __address root);
extern pte_t *find_mapping(__address page, __address root);
extern void map_structure(__address s, size_t size);

#endif
