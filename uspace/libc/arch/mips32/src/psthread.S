#
# Copyright (C) 2003-2004 Jakub Jermar
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


#include <arch/asm/regname.h>
#include <libarch/context_offset.h>
	
.global context_save
.global context_restore
	
.macro CONTEXT_STORE r
	sw $s0,OFFSET_S0(\r)
	sw $s1,OFFSET_S1(\r)
	sw $s2,OFFSET_S2(\r)
	sw $s3,OFFSET_S3(\r)
	sw $s4,OFFSET_S4(\r)
	sw $s5,OFFSET_S5(\r)
	sw $s6,OFFSET_S6(\r)
	sw $s7,OFFSET_S7(\r)
	sw $s8,OFFSET_S8(\r)
	sw $gp,OFFSET_GP(\r)
	sw $k1,OFFSET_TLS(\r)

#ifdef CONFIG_MIPS_FPU	
	mfc1 $t0,$20
	sw $t0, OFFSET_F20(\r)

	mfc1 $t0,$21
	sw $t0, OFFSET_F21(\r)

	mfc1 $t0,$22
	sw $t0, OFFSET_F22(\r)

	mfc1 $t0,$23
	sw $t0, OFFSET_F23(\r)

	mfc1 $t0,$24
	sw $t0, OFFSET_F24(\r)

	mfc1 $t0,$25
	sw $t0, OFFSET_F25(\r)

	mfc1 $t0,$26
	sw $t0, OFFSET_F26(\r)

	mfc1 $t0,$27
	sw $t0, OFFSET_F27(\r)

	mfc1 $t0,$28
	sw $t0, OFFSET_F28(\r)

	mfc1 $t0,$29
	sw $t0, OFFSET_F29(\r)
	
	mfc1 $t0,$30
	sw $t0, OFFSET_F30(\r)
#endif	
		
	sw $ra,OFFSET_PC(\r)
	sw $sp,OFFSET_SP(\r)
.endm

.macro CONTEXT_LOAD r
	lw $s0,OFFSET_S0(\r)
	lw $s1,OFFSET_S1(\r)
	lw $s2,OFFSET_S2(\r)
	lw $s3,OFFSET_S3(\r)
	lw $s4,OFFSET_S4(\r)
	lw $s5,OFFSET_S5(\r)
	lw $s6,OFFSET_S6(\r)
	lw $s7,OFFSET_S7(\r)
	lw $s8,OFFSET_S8(\r)
	lw $gp,OFFSET_GP(\r)
	lw $k1,OFFSET_TLS(\r)

#ifdef CONFIG_MIPS_FPU	
	lw $t0, OFFSET_F20(\r)
	mtc1 $t0,$20

	lw $t0, OFFSET_F21(\r)
	mtc1 $t0,$21

	lw $t0, OFFSET_F22(\r)
	mtc1 $t0,$22

	lw $t0, OFFSET_F23(\r)
	mtc1 $t0,$23

	lw $t0, OFFSET_F24(\r)
	mtc1 $t0,$24

	lw $t0, OFFSET_F25(\r)
	mtc1 $t0,$25

	lw $t0, OFFSET_F26(\r)
	mtc1 $t0,$26

	lw $t0, OFFSET_F27(\r)
	mtc1 $t0,$27

	lw $t0, OFFSET_F28(\r)
	mtc1 $t0,$28

	lw $t0, OFFSET_F29(\r)
	mtc1 $t0,$29

	lw $t0, OFFSET_F30(\r)
	mtc1 $t0,$30
#endif	
		
	lw $ra,OFFSET_PC(\r)
	lw $sp,OFFSET_SP(\r)
.endm
	
context_save:
	CONTEXT_STORE $a0

	# context_save returns 1
	j $ra
	li $v0, 1	
	
context_restore:
	CONTEXT_LOAD $a0

	# Just for the jump into first function, but one instruction
	# should not bother us
	move $t9, $ra	
	# context_restore returns 0
	j $ra
	xor $v0, $v0	

