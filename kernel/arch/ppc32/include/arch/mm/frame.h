/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup kernel_ppc32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_FRAME_H_
#define KERN_ppc32_FRAME_H_

#define FRAME_WIDTH  12  /* 4K */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <trace.h>

_NO_TRACE static inline uint32_t physmem_top(void)
{
	uint32_t physmem;

	asm volatile (
	    "mfsprg3 %[physmem]\n"
	    : [physmem] "=r" (physmem)
	);

	return physmem;
}

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
extern void physmem_print(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
