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

	lw $ra, CONTEXT_OFFSET_PC($a0)
	lw $sp, CONTEXT_OFFSET_SP($a0)

	# context_restore returns 0
	j $31
	xor $2, $2
FUNCTION_END(context_restore_arch)
