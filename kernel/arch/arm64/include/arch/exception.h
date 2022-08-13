/*
 * SPDX-FileCopyrightText: 2016 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
