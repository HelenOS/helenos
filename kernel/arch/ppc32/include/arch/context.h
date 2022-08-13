/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_CONTEXT_H_
#define KERN_ppc32_CONTEXT_H_

#include <arch/context_struct.h>

#define SP_DELTA  16

#define context_set(ctx, pc, stack, size) \
    context_set_generic(ctx, pc, stack, size)

#endif

/** @}
 */
