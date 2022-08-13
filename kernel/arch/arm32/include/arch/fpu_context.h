/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief FPU context.
 */

#ifndef KERN_arm32_FPU_CONTEXT_H_
#define KERN_arm32_FPU_CONTEXT_H_

#include <stdbool.h>
#include <stdint.h>

#define FPU_CONTEXT_ALIGN    8

/*
 * ARM Architecture reference manual, p B-1529.
 */
typedef struct {
	uint32_t fpexc;
	uint32_t fpscr;
	uint32_t s[64];
} fpu_context_t;

void fpu_setup(void);

bool handle_if_fpu_exception(void);

#endif

/** @}
 */
