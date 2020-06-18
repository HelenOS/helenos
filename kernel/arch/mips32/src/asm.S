/*
 * Copyright (c) 2003 Jakub Jermar
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
#include <arch/asm/regname.h>
#include <arch/fpu_context_struct.h>

.text

.macro cp0_read reg
	mfc0 $2, \reg
	j $31
	nop
.endm

.macro cp0_write reg
	mtc0 $4, \reg
	j $31
	nop
.endm

.set noat
.set noreorder
.set nomacro

FUNCTION_BEGIN(asm_delay_loop)
	j $31
	nop
FUNCTION_END(asm_delay_loop)

FUNCTION_BEGIN(cpu_halt)
	j cpu_halt
	nop
FUNCTION_END(cpu_halt)

FUNCTION_BEGIN(memcpy_from_uspace)
FUNCTION_BEGIN(memcpy_to_uspace)
	move $t2, $a0  /* save dst */

	addiu $v0, $a1, 3
	li $v1, -4  /* 0xfffffffffffffffc */
	and $v0, $v0, $v1
	beq $a1, $v0, 3f
	move $t0, $a0

	0:
		beq $a2, $zero, 2f
		move $a3, $zero

	1:
		addu $v0, $a1, $a3
		lbu $a0, 0($v0)
		addu $v1, $t0, $a3
		addiu $a3, $a3, 1
		bne $a3, $a2, 1b
		sb $a0, 0($v1)

	2:
		jr $ra
		move $v0, $t2

	3:
		addiu $v0, $a0, 3
		and $v0, $v0, $v1
		bne $a0, $v0, 0b
		srl $t1, $a2, 2

		beq $t1, $zero, 5f
		move $a3, $zero

		move $a3, $zero
		move $a0, $zero

	4:
		addu $v0, $a1, $a0
		lw $v1, 0($v0)
		addiu $a3, $a3, 1
		addu $v0, $t0, $a0
		sw $v1, 0($v0)
		bne $a3, $t1, 4b
		addiu $a0, $a0, 4

	5:
		andi $a2, $a2, 0x3
		beq $a2, $zero, 2b
		nop

		sll $v0, $a3, 2
		addu $t1, $v0, $t0
		move $a3, $zero
		addu $t0, $v0, $a1

	6:
		addu $v0, $t0, $a3
		lbu $a0, 0($v0)
		addu $v1, $t1, $a3
		addiu $a3, $a3, 1
		bne $a3, $a2, 6b
		sb $a0, 0($v1)

		jr $ra
		move $v0, $t2
FUNCTION_END(memcpy_from_uspace)
FUNCTION_END(memcpy_to_uspace)

SYMBOL(memcpy_from_uspace_failover_address)
SYMBOL(memcpy_to_uspace_failover_address)
	jr $ra
	move $v0, $zero

.macro fpu_gp_save reg ctx
	mfc1 $t0, $\reg
	sw $t0, FPU_CONTEXT_OFFSET_DREGS + \reg * FPU_CONTEXT_DREGS_ITEM_SIZE(\ctx)
.endm

.macro fpu_gp_restore reg ctx
	lw $t0, FPU_CONTEXT_OFFSET_DREGS + \reg * FPU_CONTEXT_DREGS_ITEM_SIZE(\ctx)
	mtc1 $t0, $\reg
.endm

.macro fpu_ct_save reg ctx
	cfc1 $t0, $1
	sw $t0, FPU_CONTEXT_OFFSET_CREGS + \reg * FPU_CONTEXT_CREGS_ITEM_SIZE(\ctx)
.endm

.macro fpu_ct_restore reg ctx
	lw $t0, FPU_CONTEXT_OFFSET_CREGS + \reg * FPU_CONTEZT_CREGS_ITEM_SIZE(\ctx)
	ctc1 $t0, $\reg
.endm

FUNCTION_BEGIN(fpu_context_save)
#ifdef CONFIG_FPU
	fpu_gp_save 0, $a0
	fpu_gp_save 1, $a0
	fpu_gp_save 2, $a0
	fpu_gp_save 3, $a0
	fpu_gp_save 4, $a0
	fpu_gp_save 5, $a0
	fpu_gp_save 6, $a0
	fpu_gp_save 7, $a0
	fpu_gp_save 8, $a0
	fpu_gp_save 9, $a0
	fpu_gp_save 10, $a0
	fpu_gp_save 11, $a0
	fpu_gp_save 12, $a0
	fpu_gp_save 13, $a0
	fpu_gp_save 14, $a0
	fpu_gp_save 15, $a0
	fpu_gp_save 16, $a0
	fpu_gp_save 17, $a0
	fpu_gp_save 18, $a0
	fpu_gp_save 19, $a0
	fpu_gp_save 20, $a0
	fpu_gp_save 21, $a0
	fpu_gp_save 22, $a0
	fpu_gp_save 23, $a0
	fpu_gp_save 24, $a0
	fpu_gp_save 25, $a0
	fpu_gp_save 26, $a0
	fpu_gp_save 27, $a0
	fpu_gp_save 28, $a0
	fpu_gp_save 29, $a0
	fpu_gp_save 30, $a0
	fpu_gp_save 31, $a0

	fpu_ct_save 1, $a0
	fpu_ct_save 2, $a0
	fpu_ct_save 3, $a0
	fpu_ct_save 4, $a0
	fpu_ct_save 5, $a0
	fpu_ct_save 6, $a0
	fpu_ct_save 7, $a0
	fpu_ct_save 8, $a0
	fpu_ct_save 9, $a0
	fpu_ct_save 10, $a0
	fpu_ct_save 11, $a0
	fpu_ct_save 12, $a0
	fpu_ct_save 13, $a0
	fpu_ct_save 14, $a0
	fpu_ct_save 15, $a0
	fpu_ct_save 16, $a0
	fpu_ct_save 17, $a0
	fpu_ct_save 18, $a0
	fpu_ct_save 19, $a0
	fpu_ct_save 20, $a0
	fpu_ct_save 21, $a0
	fpu_ct_save 22, $a0
	fpu_ct_save 23, $a0
	fpu_ct_save 24, $a0
	fpu_ct_save 25, $a0
	fpu_ct_save 26, $a0
	fpu_ct_save 27, $a0
	fpu_ct_save 28, $a0
	fpu_ct_save 29, $a0
	fpu_ct_save 30, $a0
	fpu_ct_save 31, $a0
#endif
	j $ra
	nop
FUNCTION_END(fpu_context_save)

FUNCTION_BEGIN(fpu_context_restore)
#ifdef CONFIG_FPU
	fpu_gp_restore 0, $a0
	fpu_gp_restore 1, $a0
	fpu_gp_restore 2, $a0
	fpu_gp_restore 3, $a0
	fpu_gp_restore 4, $a0
	fpu_gp_restore 5, $a0
	fpu_gp_restore 6, $a0
	fpu_gp_restore 7, $a0
	fpu_gp_restore 8, $a0
	fpu_gp_restore 9, $a0
	fpu_gp_restore 10, $a0
	fpu_gp_restore 11, $a0
	fpu_gp_restore 12, $a0
	fpu_gp_restore 13, $a0
	fpu_gp_restore 14, $a0
	fpu_gp_restore 15, $a0
	fpu_gp_restore 16, $a0
	fpu_gp_restore 17, $a0
	fpu_gp_restore 18, $a0
	fpu_gp_restore 19, $a0
	fpu_gp_restore 20, $a0
	fpu_gp_restore 21, $a0
	fpu_gp_restore 22, $a0
	fpu_gp_restore 23, $a0
	fpu_gp_restore 24, $a0
	fpu_gp_restore 25, $a0
	fpu_gp_restore 26, $a0
	fpu_gp_restore 27, $a0
	fpu_gp_restore 28, $a0
	fpu_gp_restore 29, $a0
	fpu_gp_restore 30, $a0
	fpu_gp_restore 31, $a0

	fpu_ct_restore 1, $a0
	fpu_ct_restore 2, $a0
	fpu_ct_restore 3, $a0
	fpu_ct_restore 4, $a0
	fpu_ct_restore 5, $a0
	fpu_ct_restore 6, $a0
	fpu_ct_restore 7, $a0
	fpu_ct_restore 8, $a0
	fpu_ct_restore 9, $a0
	fpu_ct_restore 10, $a0
	fpu_ct_restore 11, $a0
	fpu_ct_restore 12, $a0
	fpu_ct_restore 13, $a0
	fpu_ct_restore 14, $a0
	fpu_ct_restore 15, $a0
	fpu_ct_restore 16, $a0
	fpu_ct_restore 17, $a0
	fpu_ct_restore 18, $a0
	fpu_ct_restore 19, $a0
	fpu_ct_restore 20, $a0
	fpu_ct_restore 21, $a0
	fpu_ct_restore 22, $a0
	fpu_ct_restore 23, $a0
	fpu_ct_restore 24, $a0
	fpu_ct_restore 25, $a0
	fpu_ct_restore 26, $a0
	fpu_ct_restore 27, $a0
	fpu_ct_restore 28, $a0
	fpu_ct_restore 29, $a0
	fpu_ct_restore 30, $a0
	fpu_ct_restore 31, $a0
#endif
	j $ra
	nop
FUNCTION_END(fpu_context_restore)

FUNCTION_BEGIN(early_putuchar)
	j $ra
	nop
FUNCTION_END(early_putuchar)
