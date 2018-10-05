/*
 * Copyright (c) 2005 Martin Decky
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
