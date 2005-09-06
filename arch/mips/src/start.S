#
# Copyright (C) 2001-2004 Jakub Jermar
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

#define __ASM__
	
#include <arch/asm/regname.h>
#include <arch/mm/page.h>
#include <arch/asm/boot.h>
#include <arch/context.h>
	
.text

.set noat
.set noreorder
.set nomacro

.global kernel_image_start
.global tlb_refill_entry
.global cache_error_entry
.global exception_entry

.macro REGISTERS_STORE r
	sw $at,EOFFSET_AT(\r)
	sw $v0,EOFFSET_V0(\r)
	sw $v1,EOFFSET_V1(\r)
	sw $a0,EOFFSET_A0(\r)
	sw $a1,EOFFSET_A1(\r)
	sw $a2,EOFFSET_A2(\r)
	sw $a3,EOFFSET_A3(\r)
	sw $t0,EOFFSET_A4(\r)
	sw $t1,EOFFSET_T1(\r)
	sw $t2,EOFFSET_T2(\r)
	sw $t3,EOFFSET_T3(\r)
	sw $t4,EOFFSET_T4(\r)
	sw $t5,EOFFSET_T5(\r)
	sw $t6,EOFFSET_T6(\r)
	sw $t7,EOFFSET_T7(\r)
	sw $t8,EOFFSET_T8(\r)
	sw $t9,EOFFSET_T9(\r)
	sw $s0,EOFFSET_S0(\r)
	sw $s1,EOFFSET_S1(\r)
	sw $s2,EOFFSET_S2(\r)
	sw $s3,EOFFSET_S3(\r)
	sw $s4,EOFFSET_S4(\r)
	sw $s5,EOFFSET_S5(\r)
	sw $s6,EOFFSET_S6(\r)
	sw $s7,EOFFSET_S7(\r)
	sw $s8,EOFFSET_S8(\r)	
	sw $gp,EOFFSET_GP(\r)
	sw $ra,EOFFSET_RA(\r)
	mflo $k0
	mfhi $k1
	sw $k0,EOFFSET_LO(\r)
	sw $k1,EOFFSET_HI(\r)
.endm

.macro REGISTERS_LOAD r
	lw $at,EOFFSET_AT(\r)
	lw $v0,EOFFSET_V0(\r)
	lw $v1,EOFFSET_V1(\r)
	lw $a0,EOFFSET_A0(\r)
	lw $a1,EOFFSET_A1(\r)
	lw $a2,EOFFSET_A2(\r)
	lw $a3,EOFFSET_A3(\r)
	lw $t0,EOFFSET_A4(\r)
	lw $t1,EOFFSET_T1(\r)
	lw $t2,EOFFSET_T2(\r)
	lw $t3,EOFFSET_T3(\r)
	lw $t4,EOFFSET_T4(\r)
	lw $t5,EOFFSET_T5(\r)
	lw $t6,EOFFSET_T6(\r)
	lw $t7,EOFFSET_T7(\r)
	lw $t8,EOFFSET_T8(\r)
	lw $t9,EOFFSET_T9(\r)
	lw $s0,EOFFSET_S0(\r)
	lw $s1,EOFFSET_S1(\r)
	lw $s2,EOFFSET_S2(\r)
	lw $s3,EOFFSET_S3(\r)
	lw $s4,EOFFSET_S4(\r)
	lw $s5,EOFFSET_S5(\r)
	lw $s6,EOFFSET_S6(\r)
	lw $s7,EOFFSET_S7(\r)
	lw $s8,EOFFSET_S8(\r)
	lw $gp,EOFFSET_GP(\r)
	lw $ra,EOFFSET_RA(\r)
	
	lw $k0,EOFFSET_LO(\r)
	lw $k1,EOFFSET_HI(\r)
	mtlo $k0
	mthi $k1
.endm

	
.org 0x0
tlb_refill_entry:
	j tlb_refill_handler
	nop

.org 0x100
cache_error_entry:
	j cache_error_handler
	nop

.org 0x180
exception_entry:
exception_handler:
	sub $sp, REGISTER_SPACE
	REGISTERS_STORE $sp

	jal exception
	nop

	REGISTERS_LOAD $sp
	add $sp, REGISTER_SPACE

	eret
	nop

.org KA2PA(0x80000300)
kernel_image_start:
	/* Load temporary stack */
	lui $sp, (TEMP_STACK_START + TEMP_STACK_SIZE) >> 16
	ori $sp, (TEMP_STACK_START + TEMP_STACK_SIZE) & 0xffff

	/* Not sure about this, but might be needed for PIC code???? */
	lui $gp, 0x8000
	
	jal main_bsp
	nop

.org KA2PA(TEMP_STACK_START)
	.space TEMP_STACK_SIZE
	
tlb_refill_handler:
	sub $sp, REGISTER_SPACE
	REGISTERS_STORE $sp

	jal tlb_refill
	nop

	REGISTERS_LOAD $sp
	add $sp, REGISTER_SPACE

	eret
	nop

cache_error_handler:
	sub $sp, REGISTER_SPACE
	REGISTERS_STORE $sp

	jal cache_error
	nop

	REGISTERS_LOAD $sp
	add $sp, REGISTER_SPACE

	eret
	nop
