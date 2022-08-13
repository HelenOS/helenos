/*
 * SPDX-FileCopyrightText: 2007 Petr Stepan
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(context_save_arch)
	stmfd sp!, {r1}
	mrs r1, cpsr
	and r1, r1, #0x1f
	stmia r0!, {r1}
	ldmfd sp!, {r1}

	stmia r0!, {sp, lr}
	stmia r0!, {r4-r11}

	mov r0, #1
	mov pc, lr
FUNCTION_END(context_save_arch)

FUNCTION_BEGIN(context_restore_arch)
	ldmia r0!, {r4}
	mrs r5, cpsr
	bic r5, r5, #0x1f
	orr r5, r5, r4
	msr cpsr_c, r5

	ldmia r0!, {sp, lr}
	ldmia r0!, {r4-r11}

	mov r0, #0
	mov pc, lr
FUNCTION_END(context_restore_arch)
