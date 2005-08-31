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

#ifndef __ppc_ASM_H__
#define __ppc_ASM_H__

#include <arch/types.h>
#include <config.h>

/** Set priority level low
 *
 * Enable interrupts and return previous
 * value of EE.
 */
static inline pri_t cpu_priority_low(void) {
	pri_t v;
	__asm__ volatile (
		"mfmsr %0\n"
		"mfmsr %%r31\n"
		"ori %%r31, %%r31, 1 << 15\n"
		"mtmsr %%r31\n"
		: "=r" (v)
		:
		: "%r31"
	);
	return v;
}

/** Set priority level high
 *
 * Disable interrupts and return previous
 * value of EE.
 */
static inline pri_t cpu_priority_high(void) {
	pri_t v;
	__asm__ volatile (
		"mfmsr %0\n"
		"mfmsr %%r31\n"
		"rlwinm %%r31, %%r31, 0, 17, 15\n"
		"mtmsr %%r31\n"
		: "=r" (v)
		:
		: "%r31"
	);
	return v;
}

/** Restore priority level
 *
 * Restore EE.
 */
static inline void cpu_priority_restore(pri_t pri) {
	__asm__ volatile (
		"mfmsr %%r31\n"
		"rlwimi  %0, %%r31, 0, 17, 15\n"
		"mtmsr %0\n"
		: "=r" (pri)
		: "0" (pri)
		: "%r31"
	);
}

/* TODO: implement the real stuff */
static inline __address get_stack_base(void)
{
	return NULL;
}

void cpu_sleep(void);
void asm_delay_loop(__u32 t);

#endif
