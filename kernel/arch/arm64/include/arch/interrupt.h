/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_interrupt
 * @{
 */
/** @file
 * @brief Declarations of interrupt controlling routines.
 */

#ifndef KERN_arm64_INTERRUPT_H_
#define KERN_arm64_INTERRUPT_H_

#include <arch/istate.h>
#include <stdbool.h>
#include <typedefs.h>

/** Size of exception table. */
#define IVT_ITEMS  16

/** Index of the first item in exception table. */
#define IVT_FIRST  0

/* REVISIT */
/* This needs to be defined for inter-architecture API portability. */
#define VECTOR_TLB_SHOOTDOWN_IPI  0

extern ipl_t interrupts_disable(void);
extern ipl_t interrupts_enable(void);
extern void interrupts_restore(ipl_t ipl);
extern ipl_t interrupts_read(void);
extern bool interrupts_disabled(void);
extern void interrupt_init(void);

#endif

/** @}
 */
