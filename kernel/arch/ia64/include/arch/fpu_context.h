/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_FPU_CONTEXT_H_
#define KERN_ia64_FPU_CONTEXT_H_

#define FPU_CONTEXT_ALIGN 16

#include <_bits/int128_t.h>

#define FRS 96

typedef struct {
	uint128_t fr[FRS];
} fpu_context_t;

#endif

/** @}
 */
