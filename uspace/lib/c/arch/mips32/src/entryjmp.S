/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <libarch/stack.h>

.text
.section .text
.set noreorder

## void entry_point_jmp(void *entry_point, void *pcb);
#
# $a0 (=$4) contains entry_point
# $a1 (=$5) contains pcb
#
# Jump to program entry point
FUNCTION_BEGIN(entry_point_jmp)
	# tmp := entry_point
	move $t9, $a0

	# Pass pcb to the entry point in $a0
	move $a0, $a1

	jr $t9
	addiu $sp, -ABI_STACK_FRAME
	addiu $sp, ABI_STACK_FRAME
FUNCTION_END(entry_point_jmp)
