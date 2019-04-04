/*
 * Copyright (c) 2003-2004 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
