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

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_SUN4U_FRAME_H_
#define KERN_sparc64_SUN4U_FRAME_H_

/*
 * Page size supported by the MMU.
 * For 8K there is the nasty illegal virtual aliasing problem.
 * Therefore, the kernel uses 8K only internally on the TLB and TSB levels.
 */
#define MMU_FRAME_WIDTH  13  /* 8K */
#define MMU_FRAME_SIZE   (1 << MMU_FRAME_WIDTH)

/*
 * Page size exported to the generic memory management subsystems.
 * This page size is not directly supported by the MMU, but we can emulate
 * each 16K page with a pair of adjacent 8K pages.
 */
#define FRAME_WIDTH  14  /* 16K */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

#include <typedefs.h>

union frame_address {
	uintptr_t address;
	struct {
#if defined (US)
		unsigned : 23;
		uint64_t pfn : 28;		/**< Physical Frame Number. */
#elif defined (US3)
		unsigned : 21;
		uint64_t pfn : 30;		/**< Physical Frame Number. */
#endif
		unsigned offset : 13;		/**< Offset. */
	} __attribute__((packed));
};

typedef union frame_address frame_address_t;

#endif

#endif

/** @}
 */
