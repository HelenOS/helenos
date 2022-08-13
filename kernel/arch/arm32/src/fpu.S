/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

