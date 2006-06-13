/*
 * Copyright (C) 2005 Martin Decky
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

 /** @addtogroup ppc32	
 * @{
 */
/** @file
 */

#ifndef __ppc32_ASM_H__
#define __ppc32_ASM_H__

#include <arch/types.h>
#include <config.h>

/** Enable interrupts.
 *
 * Enable interrupts and return previous
 * value of EE.
 *
 * @return Old interrupt priority level.
 */
static inline ipl_t interrupts_enable(void)
{
	ipl_t v;
	ipl_t tmp;
	
	asm volatile (
		"mfmsr %0\n"
		"mfmsr %1\n"
		"ori %1, %1, 1 << 15\n"
		"mtmsr %1\n"
		: "=r" (v), "=r" (tmp)
	);
	return v;
}

/** Disable interrupts.
 *
 * Disable interrupts and return previous
 * value of EE.
 *
 * @return Old interrupt priority level.
 */
static inline ipl_t interrupts_disable(void)
{
	ipl_t v;
	ipl_t tmp;
	
	asm volatile (
		"mfmsr %0\n"
		"mfmsr %1\n"
		"rlwinm %1, %1, 0, 17, 15\n"
		"mtmsr %1\n"
		: "=r" (v), "=r" (tmp)
	);
	return v;
}

/** Restore interrupt priority level.
 *
 * Restore EE.
 *
 * @param ipl Saved interrupt priority level.
 */
static inline void interrupts_restore(ipl_t ipl)
{
	ipl_t tmp;
	
	asm volatile (
		"mfmsr %1\n"
		"rlwimi  %0, %1, 0, 17, 15\n"
		"cmpw 0, %0, %1\n"
		"beq 0f\n"
		"mtmsr %0\n"
		"0:\n"
		: "=r" (ipl), "=r" (tmp)
		: "0" (ipl)
		: "cr0"
	);
}

/** Return interrupt priority level.
 *
 * Return EE.
 *
 * @return Current interrupt priority level.
 */
static inline ipl_t interrupts_read(void)
{
	ipl_t v;
	
	asm volatile (
		"mfmsr %0\n"
		: "=r" (v)
	);
	return v;
}

/** Return base address of current stack.
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE bytes long.
 * The stack must start on page boundary.
 */
static inline __address get_stack_base(void)
{
	__address v;
	
	asm volatile (
		"and %0, %%sp, %1\n"
		: "=r" (v)
		: "r" (~(STACK_SIZE - 1))
	);
	return v;
}

static inline void cpu_sleep(void)
{
}

void cpu_halt(void);
void asm_delay_loop(__u32 t);

extern void userspace_asm(__address uspace_uarg, __address stack, __address entry);

#endif

 /** @}
 */

