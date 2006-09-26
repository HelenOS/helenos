/*
 * Copyright (C) 2005 Jakub Jermar
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

/*
 * We assume TSO memory model in which only reads can pass earlier stores
 * (but not earlier reads). Therefore, CS_ENTER_BARRIER() and CS_LEAVE_BARRIER()
 * can be empty.
 */
#define CS_ENTER_BARRIER()	__asm__ volatile ("" ::: "memory")
#define CS_LEAVE_BARRIER()	__asm__ volatile ("" ::: "memory")

#define memory_barrier()	__asm__ volatile ("membar #LoadLoad | #StoreStore\n" ::: "memory")
#define read_barrier()		__asm__ volatile ("membar #LoadLoad\n" ::: "memory")
#define write_barrier()		__asm__ volatile ("membar #StoreStore\n" ::: "memory")

/** Flush Instruction Memory instruction. */
static inline void flush(void)
{
	/*
	 * The FLUSH instruction takes address parameter.
	 * As such, it may trap if the address is not found in DTLB.
	 *
	 * The entire kernel text is mapped by a locked ITLB and
	 * DTLB entries. Therefore, when this function is called,
	 * the %o7 register will always be in the range mapped by
	 * DTLB.
	 */
	 
        __asm__ volatile ("flush %o7\n");
}

/** Memory Barrier instruction. */
static inline void membar(void)
{
	__asm__ volatile ("membar #Sync\n");
}

#endif

/** @}
 */
