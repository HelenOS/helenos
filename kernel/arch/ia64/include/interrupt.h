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

/** @addtogroup ia64interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_INTERRUPT_H_
#define KERN_ia64_INTERRUPT_H_

#include <typedefs.h>
#include <arch/istate.h>

/** ia64 has 256 INRs. */
#define INR_COUNT  256

/*
 * We need to keep this just to compile.
 * We might eventually move interrupt/ stuff
 * to genarch.
 */
#define IVT_ITEMS  0
#define IVT_FIRST  0

/** External Interrupt vectors. */

#define VECTOR_TLB_SHOOTDOWN_IPI  0xf0

#define INTERRUPT_SPURIOUS  15
#define INTERRUPT_TIMER     255

#define LEGACY_INTERRUPT_BASE  0x20

#define IRQ_KBD    (0x01 + LEGACY_INTERRUPT_BASE)
#define IRQ_MOUSE  (0x0c + LEGACY_INTERRUPT_BASE)

/** General Exception codes. */
#define GE_ILLEGALOP     0
#define GE_PRIVOP        1
#define GE_PRIVREG       2
#define GE_RESREGFLD     3
#define GE_DISBLDISTRAN  4
#define GE_ILLEGALDEP    8

#define EOI  0  /**< The actual value doesn't matter. */

extern void *ivt;

extern void general_exception(uint64_t, istate_t *);
extern int break_instruction(uint64_t, istate_t *);
extern void universal_handler(uint64_t, istate_t *);
extern void nop_handler(uint64_t, istate_t *);
extern void external_interrupt(uint64_t, istate_t *);
extern void disabled_fp_register(uint64_t, istate_t *);

extern void trap_virtual_enable_irqs(uint16_t);

#endif

/** @}
 */
