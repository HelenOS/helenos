/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_CPU_H_
#define KERN_mips32_CPU_H_

#include <arch/asm.h>
#include <stdint.h>

typedef struct {
	uint32_t imp_num;
	uint32_t rev_num;
} cpu_arch_t;

#endif

/** @}
 */
