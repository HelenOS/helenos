/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_FPU_CONTEXT_H_
#define KERN_FPU_CONTEXT_H_

#include <arch/fpu_context.h>

extern void fpu_context_save(fpu_context_t *);
extern void fpu_context_restore(fpu_context_t *);
extern void fpu_init(void);
extern void fpu_enable(void);
extern void fpu_disable(void);

#endif /* KERN_FPU_CONTEXT_H_ */

/** @}
 */
