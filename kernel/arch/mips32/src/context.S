/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/context_struct.h>

.text

.set noat
.set noreorder
.set nomacro

FUNCTION_BEGIN(context_save_arch)
	sw $s0, CONTEXT_OFFSET_S0($a0)
	sw $s1, CONTEXT_OFFSET_S1($a0)
	sw $s2, CONTEXT_OFFSET_S2($a0)
	sw $s3, CONTEXT_OFFSET_S3($a0)
	sw $s4, CONTEXT_OFFSET_S4($a0)
	sw $s5, CONTEXT_OFFSET_S5($a0)
	sw $s6, CONTEXT_OFFSET_S6($a0)
	sw $s7, CONTEXT_OFFSET_S7($a0)
	sw $s8, CONTEXT_OFFSET_S8($a0)
	sw $gp, CONTEXT_OFFSET_GP($a0)
	sw $k1, CONTEXT_OFFSET_TP($a0)

	sw $ra, CONTEXT_OFFSET_PC($a0)
	sw $sp, CONTEXT_OFFSET_SP($a0)

	# context_save returns 1
	j $31
	li $2, 1
FUNCTION_END(context_save_arch)

FUNCTION_BEGIN(context_restore_arch)
	lw $s0, CONTEXT_OFFSET_S0($a0)
	lw $s1, CONTEXT_OFFSET_S1($a0)
	lw $s2, CONTEXT_OFFSET_S2($a0)
	lw $s3, CONTEXT_OFFSET_S3($a0)
	lw $s4, CONTEXT_OFFSET_S4($a0)
	lw $s5, CONTEXT_OFFSET_S5($a0)
	lw $s6, CONTEXT_OFFSET_S6($a0)
	lw $s7, CONTEXT_OFFSET_S7($a0)
	lw $s8, CONTEXT_OFFSET_S8($a0)
	lw $gp, CONTEXT_OFFSET_GP($a0)
	lw $k1, CONTEXT_OFFSET_TP($a0)

	lw $ra, CONTEXT_OFFSET_PC($a0)
	lw $sp, CONTEXT_OFFSET_SP($a0)

	# context_restore returns 0
	j $31
	xor $2, $2
FUNCTION_END(context_restore_arch)
