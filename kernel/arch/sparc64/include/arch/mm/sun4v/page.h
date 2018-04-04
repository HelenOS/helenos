/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_PAGE_H_
#define KERN_sparc64_sun4v_PAGE_H_

#include <arch/mm/frame.h>

#define MMU_PAGE_WIDTH	MMU_FRAME_WIDTH
#define MMU_PAGE_SIZE	MMU_FRAME_SIZE

#define PAGE_WIDTH	FRAME_WIDTH
#define PAGE_SIZE	FRAME_SIZE

#define MMU_PAGES_PER_PAGE	(1 << (PAGE_WIDTH - MMU_PAGE_WIDTH))

#ifndef __ASSEMBLER__

#include <arch/interrupt.h>

extern uintptr_t physmem_base;

#define KA2PA(x)	(((uintptr_t) (x)) + physmem_base)
#define PA2KA(x)	(((uintptr_t) (x)) - physmem_base)

typedef union {
	uintptr_t address;
	struct {
		uint64_t vpn : 51;		/**< Virtual Page Number. */
		unsigned offset : 13;		/**< Offset. */
	} __attribute__((packed));
} page_address_t;

extern void page_arch_init(void);

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
