/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __ia64_ASM_H__
#define __ia64_ASM_H__

#include <arch/types.h>
#include <config.h>
#include <arch/register.h>

/** Return base address of current stack
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE long.
 * The stack must start on page boundary.
 */
static inline __address get_stack_base(void)
{
	__u64 v;

	__asm__ volatile ("and %0 = %1, r12" : "=r" (v) : "r" (~(STACK_SIZE-1)));
	
	return v;
}

/** Read IVA (Interruption Vector Address).
 *
 * @return Return location of interruption vector table.
 */
static inline __u64 iva_read(void)
{
	__u64 v;
	
	__asm__ volatile ("mov %0 = cr.iva\n" : "=r" (v));
	
	return v;
}

/** Write IVA (Interruption Vector Address) register.
 *
 * @param New location of interruption vector table.
 */
static inline void iva_write(__u64 v)
{
	__asm__ volatile ("mov cr.iva = %0\n" : : "r" (v));
}


/** Read IVR (External Interrupt Vector Register).
 *
 * @return Highest priority, pending, unmasked external interrupt vector.
 */
static inline __u64 ivr_read(void)
{
	__u64 v;
	
	__asm__ volatile ("mov %0 = cr.ivr\n" : "=r" (v));
	
	return v;
}

/** Write ITC (Interval Timer Counter) register.
 *
 * @param New counter value.
 */
static inline void itc_write(__u64 v)
{
	__asm__ volatile ("mov ar.itc = %0\n" : : "r" (v));
}

/** Read ITC (Interval Timer Counter) register.
 *
 * @return Current counter value.
 */
static inline __u64 itc_read(void)
{
	__u64 v;
	
	__asm__ volatile ("mov %0 = ar.itc\n" : "=r" (v));
	
	return v;
}

/** Write ITM (Interval Timer Match) register.
 *
 * @param New match value.
 */
static inline void itm_write(__u64 v)
{
	__asm__ volatile ("mov cr.itm = %0\n" : : "r" (v));
}

/** Read ITV (Interval Timer Vector) register.
 *
 * @return Current vector and mask bit.
 */
static inline __u64 itv_read(void)
{
	__u64 v;
	
	__asm__ volatile ("mov %0 = cr.itv\n" : "=r" (v));
	
	return v;
}

/** Write ITV (Interval Timer Vector) register.
 *
 * @param New vector and mask bit.
 */
static inline void itv_write(__u64 v)
{
	__asm__ volatile ("mov cr.itv = %0\n" : : "r" (v));
}

/** Write EOI (End Of Interrupt) register.
 *
 * @param This value is ignored.
 */
static inline void eoi_write(__u64 v)
{
	__asm__ volatile ("mov cr.eoi = %0\n" : : "r" (v));
}

/** Read TPR (Task Priority Register).
 *
 * @return Current value of TPR.
 */
static inline __u64 tpr_read(void)
{
	__u64 v;

	__asm__ volatile ("mov %0 = cr.tpr\n"  : "=r" (v));
	
	return v;
}

/** Write TPR (Task Priority Register).
 *
 * @param New value of TPR.
 */
static inline void tpr_write(__u64 v)
{
	__asm__ volatile ("mov cr.tpr = %0\n" : : "r" (v));
}

/** Disable interrupts.
 *
 * Disable interrupts and return previous
 * value of PSR.
 *
 * @return Old interrupt priority level.
 */
static ipl_t interrupts_disable(void)
{
	__u64 v;
	
	__asm__ volatile (
		"mov %0 = psr\n"
		"rsm %1\n"
		: "=r" (v)
		: "i" (PSR_I_MASK)
	);
	
	return (ipl_t) v;
}

/** Enable interrupts.
 *
 * Enable interrupts and return previous
 * value of PSR.
 *
 * @return Old interrupt priority level.
 */
static ipl_t interrupts_enable(void)
{
	__u64 v;
	
	__asm__ volatile (
		"mov %0 = psr\n"
		"ssm %1\n"
		";;\n"
		"srlz.d\n"
		: "=r" (v)
		: "i" (PSR_I_MASK)
	);
	
	return (ipl_t) v;
}

/** Restore interrupt priority level.
 *
 * Restore PSR.
 *
 * @param ipl Saved interrupt priority level.
 */
static inline void interrupts_restore(ipl_t ipl)
{
	if (ipl & PSR_I_MASK)
		(void) interrupts_enable();
	else
		(void) interrupts_disable();
}

/** Return interrupt priority level.
 *
 * @return PSR.
 */
static inline ipl_t interrupts_read(void)
{
	__u64 v;
	
	__asm__ volatile ("mov %0 = psr\n" : "=r" (v));
	
	return (ipl_t) v;
}

extern void cpu_halt(void);
extern void cpu_sleep(void);
extern void asm_delay_loop(__u32 t);

#endif
