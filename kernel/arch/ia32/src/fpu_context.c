/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
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
