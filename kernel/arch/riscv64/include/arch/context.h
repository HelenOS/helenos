/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_CONTEXT_H_
#define KERN_riscv64_CONTEXT_H_

#include <arch/context_struct.h>

#define SP_DELTA  16

#define context_set(context, pc, stack, size) \
	context_set_generic(context, pc, stack, size)

#endif

/** @}
 */
