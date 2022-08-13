/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_interrupt
 * @{
 */
/** @file
 *  @brief Declarations of interrupt controlling routines.
 */

#ifndef KERN_arm32_INTERRUPT_H_
#define KERN_arm32_INTERRUPT_H_

#include <stdbool.h>
#include <arch/exception.h>

/** Initial size of exception dispatch table. */
#define IVT_ITEMS  6

/** Index of the first item in exception dispatch table. */
#define IVT_FIRST  0

extern void interrupt_init(void);
extern ipl_t interrupts_disable(void);
extern ipl_t interrupts_enable(void);
extern void interrupts_restore(ipl_t ipl);
extern ipl_t interrupts_read(void);
extern bool interrupts_disabled(void);

#endif

/** @}
 */
