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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_BARRIER_H_
#define KERN_sparc64_BARRIER_H_

#include <trace.h>

/*
 * Our critical section barriers are prepared for the weakest RMO memory model.
 */
#define CS_ENTER_BARRIER() \
	asm volatile ( \
		"membar #LoadLoad | #LoadStore\n" \
		::: "memory" \
	)

#define CS_LEAVE_BARRIER() \
	asm volatile ( \
		"membar #StoreStore\n" \
		"membar #LoadStore\n" \
		::: "memory" \
	)

#define memory_barrier() \
	asm volatile ( \
		"membar #LoadLoad | #StoreStore\n" \
		::: "memory" \
	)

#define read_barrier() \
	asm volatile ( \
		"membar #LoadLoad\n" \
		::: "memory" \
	)

#define write_barrier() \
	asm volatile ( \
		"membar #StoreStore\n" \
		::: "memory" \
	)

#define flush(a) \
	asm volatile ( \
		"flush %[reg]\n" \
		:: [reg] "r" ((a)) \
		: "memory" \
	)

/** Flush Instruction pipeline. */
NO_TRACE static inline void flush_pipeline(void)
{
	unsigned long pc;

	/*
	 * The FLUSH instruction takes address parameter.
	 * As such, it may trap if the address is not found in DTLB.
	 *
	 * The entire kernel text is mapped by a locked ITLB and
	 * DTLB entries. Therefore, when this function is called,
	 * the %pc register will always be in the range mapped by
	 * DTLB.
	 *
	 */

	asm volatile (
		"rd %%pc, %[pc]\n"
		"flush %[pc]\n"
		: [pc] "=&r" (pc)
	);
}

/** Memory Barrier instruction. */
NO_TRACE static inline void membar(void)
{
	asm volatile (
		"membar #Sync\n"
	);
}

#ifdef KERNEL

#if defined(US)

#define FLUSH_INVAL_MIN  4

#define smc_coherence(a) \
	do { \
		write_barrier(); \
		flush((a)); \
	} while (0)

#define smc_coherence_block(a, l) \
	do { \
		unsigned long i; \
		write_barrier(); \
		\
		for (i = 0; i < (l); i += FLUSH_INVAL_MIN) \
			flush((void *)(a) + i); \
	} while (0)

#elif defined (US3)

#define smc_coherence(a) \
	do { \
		write_barrier(); \
		flush_pipeline(); \
	} while (0)

#define smc_coherence_block(a, l) \
	do { \
		write_barrier(); \
		flush_pipeline(); \
	} while (0)

#endif  /* defined(US3) */

#endif	/* KERNEL */

#endif

/** @}
 */
