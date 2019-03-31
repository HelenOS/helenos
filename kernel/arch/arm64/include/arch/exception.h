/*
 * Copyright (c) 2016 Petr Pavlu
 *
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Exception declarations.
 */

#ifndef KERN_arm64_EXCEPTION_H_
#define KERN_arm64_EXCEPTION_H_

/* Exception numbers. */

/** Current Exception level with SP_EL0, Synchronous. */
#define EXC_CURRENT_EL_SP_SEL0_SYNCH    0
/** Current Exception level with SP_EL0, IRQ or vIRQ. */
#define EXC_CURRENT_EL_SP_SEL0_IRQ      1
/** Current Exception level with SP_EL0, FIQ or vFIQ. */
#define EXC_CURRENT_EL_SP_SEL0_FIQ      2
/** Current Exception level with SP_EL0, SError or vSError. */
#define EXC_CURRENT_EL_SP_SEL0_SERROR   3
/** Current Exception level with SP_ELx, x > 0, Synchronous. */
#define EXC_CURRENT_EL_SP_SELX_SYNCH    4
/** Current Exception level with SP_ELx, x > 0, IRQ or vIRQ. */
#define EXC_CURRENT_EL_SP_SELX_IRQ      5
/** Current Exception level with SP_ELx, x > 0, FIQ or vFIQ. */
#define EXC_CURRENT_EL_SP_SELX_FIQ      6
/** Current Exception level with SP_ELx, x > 0, SError or vSError. */
#define EXC_CURRENT_EL_SP_SELX_SERROR   7
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch64, Synchronous.
 */
#define EXC_LOWER_EL_AARCH64_SYNCH      8
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch64, IRQ or vIRQ.
 */
#define EXC_LOWER_EL_AARCH64_IRQ        9
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch64, FIQ or vFIQ.
 */
#define EXC_LOWER_EL_AARCH64_FIQ       10
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch64, SError or vSError.
 */
#define EXC_LOWER_EL_AARCH64_SERROR    11
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch32, Synchronous.
 */
#define EXC_LOWER_EL_AARCH32_SYNCH     12
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch32, IRQ or vIRQ.
 */
#define EXC_LOWER_EL_AARCH32_IRQ       13
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch32, FIQ or vFIQ.
 */
#define EXC_LOWER_EL_AARCH32_FIQ       14
/** Lower Exception level, where the implemented level immediately lower than
 * the target level is using AArch32, SError or vSError.
 */
#define EXC_LOWER_EL_AARCH32_SERROR    15

#ifndef __ASSEMBLER__
extern void exception_init(void);
#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
