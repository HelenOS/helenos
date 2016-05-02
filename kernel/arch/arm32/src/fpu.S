/*
 * Copyright (c) 2013 Jan Vesely
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

.text

FUNCTION_BEGIN(fpsid_read)
	vmrs r0, fpsid
	mov pc, lr
FUNCTION_END(fpsid_read)

FUNCTION_BEGIN(mvfr0_read)
	vmrs r0, mvfr0
	mov pc, lr
FUNCTION_END(mvfr0_read)

FUNCTION_BEGIN(fpscr_read)
	vmrs r0, fpscr
	mov pc, lr
FUNCTION_END(fpscr_read)

FUNCTION_BEGIN(fpscr_write)
	vmsr fpscr, r0
	mov pc, lr
FUNCTION_END(fpscr_write)

FUNCTION_BEGIN(fpexc_read)
	vmrs r0, fpexc
	mov pc, lr
FUNCTION_END(fpexc_read)

FUNCTION_BEGIN(fpexc_write)
	vmsr fpexc, r0
	mov pc, lr
FUNCTION_END(fpexc_write)

FUNCTION_BEGIN(fpu_context_save_s32)
	vmrs r1, fpexc
	vmrs r2, fpscr
	stmia r0!, {r1, r2}
	vstmia r0!, {s0-s31}
	mov pc, lr
FUNCTION_END(fpu_context_save_s32)

FUNCTION_BEGIN(fpu_context_restore_s32)
	ldmia r0!, {r1, r2}
	vmsr fpexc, r1
	vmsr fpscr, r2
	vldmia r0!, {s0-s31}
	mov pc, lr
FUNCTION_END(fpu_context_restore_s32)

FUNCTION_BEGIN(fpu_context_save_d16)
	vmrs r1, fpexc
	vmrs r2, fpscr
	stmia r0!, {r1, r2}
	vstmia r0!, {d0-d15}
	mov pc, lr
FUNCTION_END(fpu_context_save_d16)

FUNCTION_BEGIN(fpu_context_restore_d16)
	ldmia r0!, {r1, r2}
	vmsr fpexc, r1
	vmsr fpscr, r2
	vldmia r0!, {d0-d15}
	mov pc, lr
FUNCTION_END(fpu_context_restore_d16)

FUNCTION_BEGIN(fpu_context_save_d32)
	vmrs r1, fpexc
	stmia r0!, {r1}
	vmrs r1, fpscr
	stmia r0!, {r1}
	vstmia r0!, {d0-d15}
	vstmia r0!, {d16-d31}
	mov pc, lr
FUNCTION_END(fpu_context_save_d32)

FUNCTION_BEGIN(fpu_context_restore_d32)
	ldmia r0!, {r1, r2}
	vmsr fpexc, r1
	vmsr fpscr, r2
	vldmia r0!, {d0-d15}
	vldmia r0!, {d16-d31}
	mov pc, lr
FUNCTION_END(fpu_context_restore_d32)

