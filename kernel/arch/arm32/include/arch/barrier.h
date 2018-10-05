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

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Memory barriers.
 */

#ifndef KERN_arm32_BARRIER_H_
#define KERN_arm32_BARRIER_H_

#include <arch/cache.h>
#include <arch/cp15.h>
#include <align.h>

#if defined PROCESSOR_ARCH_armv7_a

/*
 * ARMv7 uses instructions for memory barriers see ARM Architecture reference
 * manual for details:
 * DMB: ch. A8.8.43 page A8-376
 * DSB: ch. A8.8.44 page A8-378
 * See ch. A3.8.3 page A3-148 for details about memory barrier implementation
 * and functionality on armv7 architecture.
 */
#define dmb()    asm volatile ("dmb" ::: "memory")
#define dsb()    asm volatile ("dsb" ::: "memory")
#define isb()    asm volatile ("isb" ::: "memory")

#elif defined PROCESSOR_ARCH_armv6

/*
 * ARMv6 introduced user access of the following commands:
 * - Prefetch flush
 * - Data synchronization barrier
 * - Data memory barrier
 * - Clean and prefetch range operations.
 * ARM Architecture Reference Manual version I ch. B.3.2.1 p. B3-4
 */
/*
 * ARMv6- use system control coprocessor (CP15) for memory barrier instructions.
 * Although at least mcr p15, 0, r0, c7, c10, 4 is mentioned in earlier archs,
 * CP15 implementation is mandatory only for armv6+.
 */
#define dmb()    CP15DMB_write(0)
#define dsb()    CP15DSB_write(0)
#define isb()    CP15ISB_write(0)

#else

#define dmb()    CP15DSB_write(0)
#define dsb()    CP15DSB_write(0)
#define isb()

#endif

#endif

/** @}
 */
