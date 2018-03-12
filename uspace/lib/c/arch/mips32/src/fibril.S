#
# Copyright (c) 2003-2004 Jakub Jermar
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

.text

.set noat
.set noreorder

#include <abi/asmtool.h>
#include <libarch/fibril_context.h>

FUNCTION_BEGIN(__setjmp)
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

	sw $k1, CONTEXT_OFFSET_TLS($a0)

#ifdef CONFIG_FPU
	mfc1 $t0, $20
	sw $t0, CONTEXT_OFFSET_F20($a0)

	mfc1 $t0, $21
	sw $t0, CONTEXT_OFFSET_F21($a0)

	mfc1 $t0, $22
	sw $t0, CONTEXT_OFFSET_F22($a0)

	mfc1 $t0, $23
	sw $t0, CONTEXT_OFFSET_F23($a0)

	mfc1 $t0, $24
	sw $t0, CONTEXT_OFFSET_F24($a0)

	mfc1 $t0, $25
	sw $t0, CONTEXT_OFFSET_F25($a0)

	mfc1 $t0, $26
	sw $t0, CONTEXT_OFFSET_F26($a0)

	mfc1 $t0, $27
	sw $t0, CONTEXT_OFFSET_F27($a0)

	mfc1 $t0, $28
	sw $t0, CONTEXT_OFFSET_F28($a0)

	mfc1 $t0, $29
	sw $t0, CONTEXT_OFFSET_F29($a0)

	mfc1 $t0, $30
	sw $t0, CONTEXT_OFFSET_F30($a0)
#endif /* CONFIG_FPU */

	sw $ra, CONTEXT_OFFSET_PC($a0)
	sw $sp, CONTEXT_OFFSET_SP($a0)

	# __setjmp returns 0
	j $ra
	li $v0, 0
FUNCTION_END(__setjmp)

FUNCTION_BEGIN(__longjmp)
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
	lw $k1, CONTEXT_OFFSET_TLS($a0)

#ifdef CONFIG_FPU
	lw $t0, CONTEXT_OFFSET_F20($a0)
	mtc1 $t0, $20

	lw $t0, CONTEXT_OFFSET_F21($a0)
	mtc1 $t0, $21

	lw $t0, CONTEXT_OFFSET_F22($a0)
	mtc1 $t0, $22

	lw $t0, CONTEXT_OFFSET_F23($a0)
	mtc1 $t0, $23

	lw $t0, CONTEXT_OFFSET_F24($a0)
	mtc1 $t0, $24

	lw $t0, CONTEXT_OFFSET_F25($a0)
	mtc1 $t0, $25

	lw $t0, CONTEXT_OFFSET_F26($a0)
	mtc1 $t0, $26

	lw $t0, CONTEXT_OFFSET_F27($a0)
	mtc1 $t0, $27

	lw $t0, CONTEXT_OFFSET_F28($a0)
	mtc1 $t0, $28

	lw $t0, CONTEXT_OFFSET_F29($a0)
	mtc1 $t0, $29

	lw $t0, CONTEXT_OFFSET_F30($a0)
	mtc1 $t0, $30
#endif /* CONFIG_FPU */

	lw $ra, CONTEXT_OFFSET_PC($a0)
	lw $sp, CONTEXT_OFFSET_SP($a0)

	# Just for the jump into first function,
	# but one instruction should not bother us
	move $t9, $ra

	# __longjmp returns second argument
	j $ra
	move $v0, $a1
FUNCTION_END(__longjmp)
