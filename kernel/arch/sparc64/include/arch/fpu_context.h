/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_FPU_CONTEXT_H_
#define KERN_sparc64_FPU_CONTEXT_H_

#include <stdint.h>

#define FPU_CONTEXT_ALIGN	8

typedef struct {
	uint64_t	d[32];
	uint64_t	fsr;
} fpu_context_t;

#endif

/** @}
 */
