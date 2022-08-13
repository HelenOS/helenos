/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 *
 */

#include <fpu_context.h>

/** Save FPU (mmx, sse) context using fxsave instruction */
void fpu_context_save(fpu_context_t *fctx)
{
	asm volatile (
	    "fxsave %[fctx]\n"
	    : [fctx] "=m" (fctx->fpu)
	);
}

/** Restore FPU (mmx,sse) context using fxrstor instruction */
void fpu_context_restore(fpu_context_t *fctx)
{
	asm volatile (
	    "fxrstor %[fctx]\n"
	    : [fctx] "=m" (fctx->fpu)
	);
}

void fpu_init(void)
{
	/* TODO: Zero all SSE, MMX etc. registers */
	/*
	 * Default value of SCR register is 0x1f80,
	 * it masks all FPU exceptions
	 */
	asm volatile (
	    "fninit\n"
	);
}

/** @}
 */
