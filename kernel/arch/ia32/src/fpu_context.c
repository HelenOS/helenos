/*
 * Copyright (c) 2005 Jakub Vana
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

/** @addtogroup ia32	
 * @{
 */
/** @file
 *
 */

#include <fpu_context.h>
#include <arch.h>
#include <cpu.h>

typedef void (*fpu_context_function)(fpu_context_t *fctx);

static fpu_context_function fpu_save, fpu_restore;

static void fpu_context_f_save(fpu_context_t *fctx)
{
	asm volatile (
		"fnsave %[fctx]"
		: [fctx] "=m" (*fctx)
	);
}

static void fpu_context_f_restore(fpu_context_t *fctx)
{
	asm volatile (
		"frstor %[fctx]"
		: [fctx] "=m" (*fctx)
	);
}

static void fpu_context_fx_save(fpu_context_t *fctx)
{
	asm volatile (
		"fxsave %[fctx]"
		: [fctx] "=m" (*fctx)
	);
}

static void fpu_context_fx_restore(fpu_context_t *fctx)
{
	asm volatile (
		"fxrstor %[fctx]"
		: [fctx] "=m" (*fctx)
	);
}

/* Setup using fxsr instruction */
void fpu_fxsr(void)
{
	fpu_save=fpu_context_fx_save;
	fpu_restore=fpu_context_fx_restore;
}

/* Setup using not fxsr instruction */
void fpu_fsr(void)
{
	fpu_save = fpu_context_f_save;
	fpu_restore = fpu_context_f_restore;
}

void fpu_context_save(fpu_context_t *fctx)
{
	fpu_save(fctx);
}

void fpu_context_restore(fpu_context_t *fctx)
{
	fpu_restore(fctx);
}

void fpu_init()
{
	uint32_t help0 = 0;
	uint32_t help1 = 0;
	
	asm volatile (
		"fninit\n"
		"stmxcsr %[help0]\n"
		"mov %[help0], %[help1]\n"
		"or %[magic], %[help1]\n"
		"mov %[help1], %[help0]\n"
		"ldmxcsr %[help0]\n"
		: [help0] "+m" (help0), [help1] "+r" (help1)
		: [magic] "i" (0x1f80)
	);
}

/** @}
 */
