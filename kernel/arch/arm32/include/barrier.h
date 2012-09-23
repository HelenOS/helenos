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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Memory barriers.
 */

#ifndef KERN_arm32_BARRIER_H_
#define KERN_arm32_BARRIER_H_

/*
 * TODO: implement true ARM memory barriers for macros below.
 */
#define CS_ENTER_BARRIER()  asm volatile ("" ::: "memory")
#define CS_LEAVE_BARRIER()  asm volatile ("" ::: "memory")

#define memory_barrier()  asm volatile ("" ::: "memory")
#define read_barrier()    asm volatile ("" ::: "memory")
#define write_barrier()   asm volatile ("" ::: "memory")

/*
 * There are multiple ways ICache can be implemented on ARM machines. Namely
 * PIPT, VIPT, and ASID and VMID tagged VIVT (see ARM Architecture Reference
 * Manual B3.11.2 (p. 1383).  However, CortexA8 Manual states: "For maximum
 * compatibility across processors, ARM recommends that operating systems target
 * the ARMv7 base architecture that uses ASID-tagged VIVT instruction caches,
 * and do not assume the presence of the IVIPT extension. Software that relies
 * on the IVIPT extension might fail in an unpredictable way on an ARMv7
 * implementation that does not include the IVIPT extension." (7.2.6 p. 245).
 * Only PIPT invalidates cache for all VA aliases if one block is invalidated.
 *
 * @note: Supporting ASID and VMID tagged VIVT may need to add ICache
 * maintenance to other places than just smc.
 */

/* Available on both all supported arms,
 * invalidates entire ICache so the written value does not matter. */
#define smc_coherence(a) asm volatile ( "mcr p15, 0, r0, c7, c5, 0")
#define smc_coherence_block(a, l) smc_coherence(a)


#endif

/** @}
 */
