/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt sparc64
 * @ingroup interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_INTERRUPT_H_
#define KERN_sparc64_INTERRUPT_H_

#include <arch/istate.h>

#define IVT_ITEMS  512
#define IVT_FIRST  0

/* This needs to be defined for inter-architecture API portability. */
#define VECTOR_TLB_SHOOTDOWN_IPI  0

enum {
	IPI_TLB_SHOOTDOWN = VECTOR_TLB_SHOOTDOWN_IPI,
};

extern void exc_arch_init(void);

#endif

/** @}
 */
