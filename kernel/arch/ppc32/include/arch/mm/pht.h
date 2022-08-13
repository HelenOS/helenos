/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_PHT_H_
#define KERN_ppc32_PHT_H_

#include <arch/interrupt.h>
#include <typedefs.h>

/* Forward declaration. */
struct as;

extern void pht_init(void);
extern void pht_refill(unsigned int, istate_t *);
extern void pht_invalidate(struct as *, uintptr_t, size_t);

#endif

/** @}
 */
