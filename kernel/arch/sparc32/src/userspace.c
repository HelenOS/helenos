/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup sparc32
 * @{
 */
/** @file
 */

#include <userspace.h>
#include <typedefs.h>
#include <arch.h>
#include <arch/asm.h>
#include <abi/proc/uarg.h>
#include <mm/as.h>

void userspace(uspace_arg_t *kernel_uarg)
{
	uint32_t psr = psr_read();
	psr &= ~(1 << 7);
	psr &= ~(1 << 6);
	
	/* Read invalid window variables */
	uint32_t l0;
	uint32_t l1;
	uint32_t l2;
	read_from_invalid(&l0, &l1, &l2);
	
	/* Make current window invalid */
	uint8_t wim = (psr & 0x7) + 1;
	wim = (1 << wim) | (1 >> (8 - wim));
	
	asm volatile (
		"flush\n"
		"mov %[stack], %%sp\n"
		"mov %[wim], %%wim\n"
		"ld %[v0], %%o0\n"
		"ld %[v1], %%o1\n"
		"ld %[v2], %%o2\n"
		"call write_to_invalid\n"
		"nop\n"
		"ld %[arg], %%o1\n"
		"jmp %[entry]\n"
		"mov %[psr], %%psr\n"
		:: [entry] "r" (kernel_uarg->uspace_entry),
		   [arg] "m" (kernel_uarg->uspace_uarg),
		   [psr] "r" (psr),
		   [wim] "r" ((uint32_t)wim),
		   [v0] "m" (l0),
		   [v1] "m" (l1),
		   [v2] "m" (l2),
		   [stack] "r" (kernel_uarg->uspace_stack +
		   kernel_uarg->uspace_stack_size - 64)
		: "%g3", "%g4"
	);
	
	while (1);
}

/** @}
 */
