/*
 * Copyright (C) 2005 Martin Decky
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

#ifndef __ppc32_PAGE_H__
#define __ppc32_PAGE_H__

#include <mm/page.h>
#include <arch/mm/frame.h>
#include <arch/types.h>

#define PAGE_SIZE	FRAME_SIZE

#define KA2PA(x)	(((__address) (x)) - 0x80000000)
#define PA2KA(x)	(((__address) (x)) + 0x80000000)

#define PTL0_INDEX_ARCH(vaddr)		0
#define PTL1_INDEX_ARCH(vaddr)		0
#define PTL2_INDEX_ARCH(vaddr)		0
#define PTL3_INDEX_ARCH(vaddr)		0

#define GET_PTL0_ADDRESS_ARCH()		0
#define SET_PTL0_ADDRESS_ARCH(ptl0)

#define GET_PTL1_ADDRESS_ARCH(ptl0, i)		((pte_t *) 0)
#define GET_PTL2_ADDRESS_ARCH(ptl1, i)		((pte_t *) 0)
#define GET_PTL3_ADDRESS_ARCH(ptl2, i)		((pte_t *) 0)
#define GET_FRAME_ADDRESS_ARCH(ptl3, i)		((pte_t *) 0)

#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a)
#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a)

#define GET_PTL1_FLAGS_ARCH(ptl0, i)		0
#define GET_PTL2_FLAGS_ARCH(ptl1, i)		0
#define GET_PTL3_FLAGS_ARCH(ptl2, i)		0
#define GET_FRAME_FLAGS_ARCH(ptl3, i)		0

#define SET_PTL1_FLAGS_ARCH(ptl0, i, x)
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x)

extern void page_arch_init(void);

typedef __u32 pte_t;

#endif
