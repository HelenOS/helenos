/*
 * Copyright (c) 2018 Petr Pavlu
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

#include <abi/asmtool.h>
#include <arch/fpu_context_struct.h>

.text

FUNCTION_BEGIN(fpu_context_save)
	/* Save FPU registers into fpu_context_t pointed by x0. */
	stp q0, q1, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 0]
	stp q2, q3, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 2]
	stp q4, q5, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 4]
	stp q6, q7, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 6]
	stp q8, q9, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 8]
	stp q10, q11, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 10]
	stp q12, q13, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 12]
	stp q14, q15, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 14]
	stp q16, q17, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 16]
	stp q18, q19, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 18]
	stp q20, q21, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 20]
	stp q22, q23, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 22]
	stp q24, q25, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 24]
	stp q26, q27, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 26]
	stp q28, q29, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 28]
	stp q30, q31, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 30]
	mrs x1, fpcr
	str w1, [x0, #FPU_CONTEXT_OFFSET_FPCR]
	mrs x1, fpsr
	str w1, [x0, #FPU_CONTEXT_OFFSET_FPSR]
	ret
FUNCTION_END(fpu_context_save)

FUNCTION_BEGIN(fpu_context_restore)
	/* Restore FPU registers from fpu_context_t pointed by x0. */
	ldp q0, q1, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 0]
	ldp q2, q3, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 2]
	ldp q4, q5, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 4]
	ldp q6, q7, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 6]
	ldp q8, q9, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 8]
	ldp q10, q11, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 10]
	ldp q12, q13, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 12]
	ldp q14, q15, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 14]
	ldp q16, q17, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 16]
	ldp q18, q19, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 18]
	ldp q20, q21, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 20]
	ldp q22, q23, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 22]
	ldp q24, q25, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 24]
	ldp q26, q27, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 26]
	ldp q28, q29, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 28]
	ldp q30, q31, [x0, #FPU_CONTEXT_OFFSET_VREGS + 16 * 30]
	ldr w1, [x0, #FPU_CONTEXT_OFFSET_FPCR]
	msr fpcr, x1
	ldr w1, [x0, #FPU_CONTEXT_OFFSET_FPSR]
	msr fpsr, x1
	ret
FUNCTION_END(fpu_context_restore)
