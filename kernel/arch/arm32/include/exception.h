/*
 * Copyright (c) 2007 Michal Kebrt, Petr Stepan
 *
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

/** @addtogroup arm32	
 * @{
 */
/** @file
 *  @brief Exception declarations.
 */

#ifndef KERN_arm32_EXCEPTION_H_
#define KERN_arm32_EXCEPTION_H_

#include <arch/types.h>
#include <arch/regutils.h>

/** If defined, forces using of high exception vectors. */
#define HIGH_EXCEPTION_VECTORS

#ifdef HIGH_EXCEPTION_VECTORS
	#define EXC_BASE_ADDRESS	0xffff0000
#else
	#define EXC_BASE_ADDRESS	0x0
#endif

/* Exception Vectors */
#define EXC_RESET_VEC          (EXC_BASE_ADDRESS + 0x0)
#define EXC_UNDEF_INSTR_VEC    (EXC_BASE_ADDRESS + 0x4)
#define EXC_SWI_VEC            (EXC_BASE_ADDRESS + 0x8)
#define EXC_PREFETCH_ABORT_VEC (EXC_BASE_ADDRESS + 0xc)
#define EXC_DATA_ABORT_VEC     (EXC_BASE_ADDRESS + 0x10)
#define EXC_IRQ_VEC            (EXC_BASE_ADDRESS + 0x18)
#define EXC_FIQ_VEC            (EXC_BASE_ADDRESS + 0x1c)

/* Exception numbers */
#define EXC_RESET           0
#define EXC_UNDEF_INSTR     1
#define EXC_SWI             2
#define EXC_PREFETCH_ABORT  3
#define EXC_DATA_ABORT      4
#define EXC_IRQ             5
#define EXC_FIQ             6


/** Kernel stack pointer.
 *
 * It is set when thread switches to user mode,
 * and then used for exception handling.
 */
extern uintptr_t supervisor_sp;


/** Temporary exception stack pointer.
 *
 * Temporary stack is used in exceptions handling routines
 * before switching to thread's kernel stack.
 */
extern uintptr_t exc_stack;


/** Struct representing CPU state saved when an exception occurs. */
typedef struct {
	uint32_t spsr;
	uint32_t sp;
	uint32_t lr;

	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;

	uint32_t pc;
} istate_t;


/** Sets Program Counter member of given istate structure.
 *
 * @param istate istate structure
 * @param retaddr new value of istate's PC member
 */
static inline void istate_set_retaddr(istate_t *istate, uintptr_t retaddr)
{
 	istate->pc = retaddr;
}


/** Returns true if exception happened while in userspace. */
static inline int istate_from_uspace(istate_t *istate)
{
 	return (istate->spsr & STATUS_REG_MODE_MASK) == USER_MODE;
}


/** Returns Program Counter member of given istate structure. */
static inline unative_t istate_get_pc(istate_t *istate)
{
 	return istate->pc;
}


extern void install_exception_handlers(void);
extern void exception_init(void);
extern void print_istate(istate_t *istate);
extern void reset_exception_entry(void);
extern void irq_exception_entry(void);
extern void fiq_exception_entry(void);
extern void undef_instr_exception_entry(void);
extern void prefetch_abort_exception_entry(void);
extern void data_abort_exception_entry(void);
extern void swi_exception_entry(void);


#endif

/** @}
 */
