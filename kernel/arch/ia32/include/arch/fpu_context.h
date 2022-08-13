/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_FPU_CONTEXT_H_
#define KERN_ia32_FPU_CONTEXT_H_

#include <stdint.h>

#define FPU_CONTEXT_ALIGN  16

typedef struct {
	uint8_t fpu[512];  /* FXSAVE & FXRSTOR storage area */
} fpu_context_t;

extern void fpu_fxsr(void);
extern void fpu_fsr(void);

#endif

/** @}
 */
