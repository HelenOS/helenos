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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_BARRIER_H_
#define KERN_ia32_BARRIER_H_

#include <trace.h>

/*
 * NOTE:
 * No barriers for critical section (i.e. spinlock) on IA-32 are needed:
 * - spinlock_lock() and spinlock_trylock() use serializing XCHG instruction
 * - writes cannot pass reads on IA-32 => spinlock_unlock() needs no barriers
 */

/*
 * Provisions are made to prevent compiler from reordering instructions itself.
 */

#define CS_ENTER_BARRIER()  asm volatile ("" ::: "memory")
#define CS_LEAVE_BARRIER()  asm volatile ("" ::: "memory")

NO_TRACE static inline void cpuid_serialization(void)
{
#ifndef __PIC__
	asm volatile (
	    "xorl %%eax, %%eax\n"
	    "cpuid\n"
	    ::: "eax", "ebx", "ecx", "edx", "memory"
	);
#else
	/* Must not clobber PIC register ebx */
	asm volatile (
	    "movl %%ebx, %%esi\n"
	    "xorl %%eax, %%eax\n"
	    "cpuid\n"
	    "movl %%esi, %%ebx\n"
	    ::: "eax", "ecx", "edx", "esi", "memory"
	);
#endif
}

#if defined(CONFIG_FENCES_P4)
#define memory_barrier()  asm volatile ("mfence\n" ::: "memory")
#define read_barrier()    asm volatile ("lfence\n" ::: "memory")
#ifdef CONFIG_WEAK_MEMORY
#define write_barrier()  asm volatile ("sfence\n" ::: "memory")
#else
#define write_barrier()  asm volatile ("" ::: "memory");
#endif
#elif defined(CONFIG_FENCES_P3)
#define memory_barrier()  cpuid_serialization()
#define read_barrier()    cpuid_serialization()
#ifdef CONFIG_WEAK_MEMORY
#define write_barrier()  asm volatile ("sfence\n" ::: "memory")
#else
#define write_barrier()  asm volatile ("" ::: "memory");
#endif
#else
#define memory_barrier()  cpuid_serialization()
#define read_barrier()    cpuid_serialization()
#ifdef CONFIG_WEAK_MEMORY
#define write_barrier()  cpuid_serialization()
#else
#define write_barrier()  asm volatile ("" ::: "memory");
#endif
#endif

#ifdef KERNEL

/*
 * On ia32, the hardware takes care about instruction and data cache coherence,
 * even on SMP systems.  We issue a write barrier to be sure that writes
 * queueing in the store buffer drain to the memory (even though it would be
 * sufficient for them to drain to the D-cache).
 */
#define smc_coherence(a)           write_barrier()
#define smc_coherence_block(a, l)  write_barrier()

#endif	/* KERNEL */

#endif

/** @}
 */
