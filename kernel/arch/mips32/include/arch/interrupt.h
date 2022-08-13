/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_INTERRUPT_H_
#define KERN_mips32_INTERRUPT_H_

#include <arch/exception.h>

#define IVT_ITEMS  32
#define IVT_FIRST  0

#define VECTOR_TLB_SHOOTDOWN_IPI  EXC_Int

extern function virtual_timer_fnc;
extern uint32_t count_hi;

extern void interrupt_init(void);

#endif

/** @}
 */
