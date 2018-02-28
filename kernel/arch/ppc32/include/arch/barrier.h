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

/** @addtogroup ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_BARRIER_H_
#define KERN_ppc32_BARRIER_H_

#include <trace.h>

#define CS_ENTER_BARRIER()  asm volatile ("" ::: "memory")
#define CS_LEAVE_BARRIER()  asm volatile ("" ::: "memory")

#define memory_barrier()  asm volatile ("sync" ::: "memory")
#define read_barrier()    asm volatile ("sync" ::: "memory")
#define write_barrier()   asm volatile ("eieio" ::: "memory")

#define instruction_barrier() \
	asm volatile ( \
		"sync\n" \
		"isync\n" \
	)

#ifdef KERNEL

#define COHERENCE_INVAL_MIN  4

/*
 * The IMB sequence used here is valid for all possible cache models
 * on uniprocessor. SMP might require a different sequence.
 * See PowerPC Programming Environment for 32-Bit Microprocessors,
 * chapter 5.1.5.2
 */

NO_TRACE static inline void smc_coherence(void *addr)
{
	asm volatile (
		"dcbst 0, %[addr]\n"
		"sync\n"
		"icbi 0, %[addr]\n"
		"sync\n"
		"isync\n"
		:: [addr] "r" (addr)
	);
}

NO_TRACE static inline void smc_coherence_block(void *addr, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i += COHERENCE_INVAL_MIN)
		asm volatile (
			"dcbst 0, %[addr]\n"
			:: [addr] "r" (addr + i)
		);

	memory_barrier();

	for (i = 0; i < len; i += COHERENCE_INVAL_MIN)
		asm volatile (
			"icbi 0, %[addr]\n"
			:: [addr] "r" (addr + i)
		);

	instruction_barrier();
}

#endif	/* KERNEL */

#endif

/** @}
 */
