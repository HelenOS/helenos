/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_debug
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_DEBUG_H_
#define KERN_mips32_DEBUG_H_

/** Enter the simulator trace mode */
#define ___traceon()  asm volatile ( "\t.word\t0x39\n");

/** Leave the simulator trace mode */
#define ___traceoff()  asm volatile ( "\t.word\t0x3d\n");

/** Ask the simulator to dump registers */
#define ___regview()  asm volatile ( "\t.word\t0x37\n");

/** Halt the simulator */
#define ___halt()  asm volatile ( "\t.word\t0x28\n");

/** Enter the simulator interactive mode */
#define ___intmode()  asm volatile ( "\t.word\t0x29\n");

#endif

/** @}
 */
