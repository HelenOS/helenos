/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_EXCEPTION_H_
#define KERN_mips32_EXCEPTION_H_

#include <arch/istate.h>

#define EXC_Int    0
#define EXC_Mod    1
#define EXC_TLBL   2
#define EXC_TLBS   3
#define EXC_AdEL   4
#define EXC_AdES   5
#define EXC_IBE    6
#define EXC_DBE    7
#define EXC_Sys    8
#define EXC_Bp     9
#define EXC_RI     10
#define EXC_CpU    11
#define EXC_Ov     12
#define EXC_Tr     13
#define EXC_VCEI   14
#define EXC_FPE    15
#define EXC_WATCH  23
#define EXC_VCED   31

#define INT_SW0    0
#define INT_SW1    1
#define INT_HW0    2
#define INT_HW1    3
#define INT_HW2    4
#define INT_HW3    5
#define INT_HW4    6
#define INT_TIMER  7

#define MIPS_INTERRUPTS    8
#define HW_INTERRUPTS      (MIPS_INTERRUPTS - 3)

typedef void (*int_handler_t)(unsigned int);
extern int_handler_t int_handler[];

extern void exception(istate_t *istate);
extern void tlb_refill_entry(void);
extern void exception_entry(void);
extern void cache_error_entry(void);
extern void exception_init(void);

#endif

/** @}
 */
