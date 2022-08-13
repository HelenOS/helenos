/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(__context_save)
	stmia r0!, {sp, lr}
	stmia r0!, {r4-r11}

	# return 0
	mov r0, #0
	mov pc, lr
FUNCTION_END(__context_save)

FUNCTION_BEGIN(__context_restore)
	ldmia r0!, {sp, lr}
	ldmia r0!, {r4-r11}

	# return second argument
	mov r0, r1
	mov pc, lr
FUNCTION_END(__context_restore)

