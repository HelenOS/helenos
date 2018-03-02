/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 *
 */

#include <fpu_context.h>
#include <arch/register.h>
#include <arch/asm.h>

void fpu_context_save(fpu_context_t *fctx)
{
	asm volatile (
		"std %%f0, %0\n"
		"std %%f2, %1\n"
		"std %%f4, %2\n"
		"std %%f6, %3\n"
		"std %%f8, %4\n"
		"std %%f10, %5\n"
		"std %%f12, %6\n"
		"std %%f14, %7\n"
		"std %%f16, %8\n"
		"std %%f18, %9\n"
		"std %%f20, %10\n"
		"std %%f22, %11\n"
		"std %%f24, %12\n"
		"std %%f26, %13\n"
		"std %%f28, %14\n"
		"std %%f30, %15\n"
		: "=m" (fctx->d[0]), "=m" (fctx->d[1]), "=m" (fctx->d[2]), "=m" (fctx->d[3]),
		  "=m" (fctx->d[4]), "=m" (fctx->d[5]), "=m" (fctx->d[6]), "=m" (fctx->d[7]),
		  "=m" (fctx->d[8]), "=m" (fctx->d[9]), "=m" (fctx->d[10]), "=m" (fctx->d[11]),
		  "=m" (fctx->d[12]), "=m" (fctx->d[13]), "=m" (fctx->d[14]), "=m" (fctx->d[15])
	);

	/*
	 * We need to split loading of the floating-point registers because
	 * GCC (4.1.1) can't handle more than 30 operands in one asm statement.
	 */

	asm volatile (
		"std %%f32, %0\n"
		"std %%f34, %1\n"
		"std %%f36, %2\n"
		"std %%f38, %3\n"
		"std %%f40, %4\n"
		"std %%f42, %5\n"
		"std %%f44, %6\n"
		"std %%f46, %7\n"
		"std %%f48, %8\n"
		"std %%f50, %9\n"
		"std %%f52, %10\n"
		"std %%f54, %11\n"
		"std %%f56, %12\n"
		"std %%f58, %13\n"
		"std %%f60, %14\n"
		"std %%f62, %15\n"
		: "=m" (fctx->d[16]), "=m" (fctx->d[17]), "=m" (fctx->d[18]), "=m" (fctx->d[19]),
		  "=m" (fctx->d[20]), "=m" (fctx->d[21]), "=m" (fctx->d[22]), "=m" (fctx->d[23]),
		  "=m" (fctx->d[24]), "=m" (fctx->d[25]), "=m" (fctx->d[26]), "=m" (fctx->d[27]),
		  "=m" (fctx->d[28]), "=m" (fctx->d[29]), "=m" (fctx->d[30]), "=m" (fctx->d[31])
	);

	asm volatile ("stx %%fsr, %0\n" : "=m" (fctx->fsr));
}

void fpu_context_restore(fpu_context_t *fctx)
{
	asm volatile (
		"ldd %0, %%f0\n"
		"ldd %1, %%f2\n"
		"ldd %2, %%f4\n"
		"ldd %3, %%f6\n"
		"ldd %4, %%f8\n"
		"ldd %5, %%f10\n"
		"ldd %6, %%f12\n"
		"ldd %7, %%f14\n"
		"ldd %8, %%f16\n"
		"ldd %9, %%f18\n"
		"ldd %10, %%f20\n"
		"ldd %11, %%f22\n"
		"ldd %12, %%f24\n"
		"ldd %13, %%f26\n"
		"ldd %14, %%f28\n"
		"ldd %15, %%f30\n"
		:
		: "m" (fctx->d[0]), "m" (fctx->d[1]), "m" (fctx->d[2]), "m" (fctx->d[3]),
		  "m" (fctx->d[4]), "m" (fctx->d[5]), "m" (fctx->d[6]), "m" (fctx->d[7]),
		  "m" (fctx->d[8]), "m" (fctx->d[9]), "m" (fctx->d[10]), "m" (fctx->d[11]),
		  "m" (fctx->d[12]), "m" (fctx->d[13]), "m" (fctx->d[14]), "m" (fctx->d[15])
	);

	/*
	 * We need to split loading of the floating-point registers because
	 * GCC (4.1.1) can't handle more than 30 operands in one asm statement.
	 */

	asm volatile (
		"ldd %0, %%f32\n"
		"ldd %1, %%f34\n"
		"ldd %2, %%f36\n"
		"ldd %3, %%f38\n"
		"ldd %4, %%f40\n"
		"ldd %5, %%f42\n"
		"ldd %6, %%f44\n"
		"ldd %7, %%f46\n"
		"ldd %8, %%f48\n"
		"ldd %9, %%f50\n"
		"ldd %10, %%f52\n"
		"ldd %11, %%f54\n"
		"ldd %12, %%f56\n"
		"ldd %13, %%f58\n"
		"ldd %14, %%f60\n"
		"ldd %15, %%f62\n"
		:
		: "m" (fctx->d[16]), "m" (fctx->d[17]), "m" (fctx->d[18]), "m" (fctx->d[19]),
		  "m" (fctx->d[20]), "m" (fctx->d[21]), "m" (fctx->d[22]), "m" (fctx->d[23]),
		  "m" (fctx->d[24]), "m" (fctx->d[25]), "m" (fctx->d[26]), "m" (fctx->d[27]),
		  "m" (fctx->d[28]), "m" (fctx->d[29]), "m" (fctx->d[30]), "m" (fctx->d[31])
	);

	asm volatile ("ldx %0, %%fsr\n" : : "m" (fctx->fsr));
}

void fpu_enable(void)
{
	pstate_reg_t pstate;

	pstate.value = pstate_read();
	pstate.pef = true;
	pstate_write(pstate.value);
}

void fpu_disable(void)
{
	pstate_reg_t pstate;

	pstate.value = pstate_read();
	pstate.pef = false;
	pstate_write(pstate.value);
}

void fpu_init(void)
{
	fpu_enable();
}

/** @}
 */
