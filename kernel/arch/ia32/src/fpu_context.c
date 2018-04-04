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

/** x87 FPU scr values (P3+ MMX2) */
enum {
	X87_FLUSH_ZERO_FLAG = (1 << 15),
	X87_ROUND_CONTROL_MASK = (0x3 << 13),
	x87_ROUND_TO_NEAREST_EVEN = (0x0 << 13),
	X87_ROUND_DOWN_TO_NEG_INF = (0x1 << 13),
	X87_ROUND_UP_TO_POS_INF = (0x2 << 13),
	X87_ROUND_TO_ZERO = (0x3 << 13),
	X87_PRECISION_MASK = (1 << 12),
	X87_UNDERFLOW_MASK = (1 << 11),
	X87_OVERFLOW_MASK = (1 << 10),
	X87_ZERO_DIV_MASK = (1 << 9),
	X87_DENORMAL_OP_MASK = (1 << 8),
	X87_INVALID_OP_MASK = (1 << 7),
	X87_DENOM_ZERO_FLAG = (1 << 6),
	X87_PRECISION_EXC_FLAG = (1 << 5),
	X87_UNDERFLOW_EXC_FLAG = (1 << 4),
	X87_OVERFLOW_EXC_FLAG = (1 << 3),
	X87_ZERO_DIV_EXC_FLAG = (1 << 2),
	X87_DENORMAL_EXC_FLAG = (1 << 1),
	X87_INVALID_OP_EXC_FLAG = (1 << 0),

	X87_ALL_MASK = X87_PRECISION_MASK | X87_UNDERFLOW_MASK | X87_OVERFLOW_MASK | X87_ZERO_DIV_MASK | X87_DENORMAL_OP_MASK | X87_INVALID_OP_MASK,
};

typedef void (*fpu_context_function)(fpu_context_t *fctx);

static fpu_context_function fpu_save;
static fpu_context_function fpu_restore;

static void fpu_context_f_save(fpu_context_t *fctx)
{
	asm volatile (
	    "fnsave %[fctx]"
	    : [fctx] "=m" (fctx->fpu)
	);
}

static void fpu_context_f_restore(fpu_context_t *fctx)
{
	asm volatile (
	    "frstor %[fctx]"
	    : [fctx] "=m" (fctx->fpu)
	);
}

static void fpu_context_fx_save(fpu_context_t *fctx)
{
	asm volatile (
	    "fxsave %[fctx]"
	    : [fctx] "=m" (fctx->fpu)
	);
}

static void fpu_context_fx_restore(fpu_context_t *fctx)
{
	asm volatile (
	    "fxrstor %[fctx]"
	    : [fctx] "=m" (fctx->fpu)
	);
}

/* Setup using fxsr instruction */
void fpu_fxsr(void)
{
	fpu_save = fpu_context_fx_save;
	fpu_restore = fpu_context_fx_restore;
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

/** Initialize x87 FPU. Mask all exceptions. */
void fpu_init(void)
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
	    : [magic] "i" (X87_ALL_MASK)
	);
}

/** @}
 */
