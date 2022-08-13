/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_FPU_CONTEXT_H_
#define KERN_abs32le_FPU_CONTEXT_H_

#define FPU_CONTEXT_ALIGN  16

/*
 * On real hardware this stores the FPU registers
 * which are part of the CPU context.
 */
typedef struct {
} fpu_context_t;

#endif

/** @}
 */
