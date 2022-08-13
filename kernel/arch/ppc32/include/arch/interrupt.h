/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_INTERRUPT_H_
#define KERN_ppc32_INTERRUPT_H_

#include <arch/istate.h>

#define IVT_ITEMS  16
#define IVT_FIRST  0

#define VECTOR_DATA_STORAGE         2
#define VECTOR_INSTRUCTION_STORAGE  3
#define VECTOR_EXTERNAL             4
#define VECTOR_FP_UNAVAILABLE       7
#define VECTOR_DECREMENTER          8
#define VECTOR_ITLB_MISS            13
#define VECTOR_DTLB_MISS_LOAD       14
#define VECTOR_DTLB_MISS_STORE      15

extern void decrementer_start(uint32_t);
extern void decrementer_restart(void);
extern void interrupt_init(void);
extern void extint_handler(unsigned int, istate_t *);

#endif

/** @}
 */
