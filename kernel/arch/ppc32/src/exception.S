#
# Copyright (c) 2006 Martin Decky
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
#include <arch/asm/regname.h>
#include <arch/msr.h>
#include <arch/mm/page.h>
#include <arch/istate_struct.h>
#include <arch/stack.h>
#include <align.h>

.section K_UNMAPPED_TEXT_START, "ax"

.macro CONTEXT_STORE

	# save r12 in SPRG1, backup CR in r12
	# save SP in SPRG2

	mtsprg1 r12
	mfcr r12
	mtsprg2 sp

	# check whether the previous mode was user or kernel

	mfsrr1 sp # use sp as a temporary register to hold SRR1
	andi. sp, sp, MSR_PR
	bne 1f
		# previous mode was kernel

		mfsprg2 sp
		subis sp, sp, 0x8000
	b 2f

	1:
		# previous mode was user

		mfsprg0 sp
	2:

	subi sp, sp, ALIGN_UP(ISTATE_SIZE, STACK_ALIGNMENT)
	stw r0, ISTATE_OFFSET_R0(sp)
	stw r2, ISTATE_OFFSET_R2(sp)
	stw r3, ISTATE_OFFSET_R3(sp)
	stw r4, ISTATE_OFFSET_R4(sp)
	stw r5, ISTATE_OFFSET_R5(sp)
	stw r6, ISTATE_OFFSET_R6(sp)
	stw r7, ISTATE_OFFSET_R7(sp)
	stw r8, ISTATE_OFFSET_R8(sp)
	stw r9, ISTATE_OFFSET_R9(sp)
	stw r10, ISTATE_OFFSET_R10(sp)
	stw r11, ISTATE_OFFSET_R11(sp)
	stw r13, ISTATE_OFFSET_R13(sp)
	stw r14, ISTATE_OFFSET_R14(sp)
	stw r15, ISTATE_OFFSET_R15(sp)
	stw r16, ISTATE_OFFSET_R16(sp)
	stw r17, ISTATE_OFFSET_R17(sp)
	stw r18, ISTATE_OFFSET_R18(sp)
	stw r19, ISTATE_OFFSET_R19(sp)
	stw r20, ISTATE_OFFSET_R20(sp)
	stw r21, ISTATE_OFFSET_R21(sp)
	stw r22, ISTATE_OFFSET_R22(sp)
	stw r23, ISTATE_OFFSET_R23(sp)
	stw r24, ISTATE_OFFSET_R24(sp)
	stw r25, ISTATE_OFFSET_R25(sp)
	stw r26, ISTATE_OFFSET_R26(sp)
	stw r27, ISTATE_OFFSET_R27(sp)
	stw r28, ISTATE_OFFSET_R28(sp)
	stw r29, ISTATE_OFFSET_R29(sp)
	stw r30, ISTATE_OFFSET_R30(sp)
	stw r31, ISTATE_OFFSET_R31(sp)

	stw r12, ISTATE_OFFSET_CR(sp)

	mfsrr0 r12
	stw r12, ISTATE_OFFSET_PC(sp)

	mfsrr1 r12
	stw r12, ISTATE_OFFSET_SRR1(sp)

	mflr r12
	stw r12, ISTATE_OFFSET_LR(sp)

	mfctr r12
	stw r12, ISTATE_OFFSET_CTR(sp)

	mfxer r12
	stw r12, ISTATE_OFFSET_XER(sp)

	mfdar r12
	stw r12, ISTATE_OFFSET_DAR(sp)

	mfsprg1 r12
	stw r12, ISTATE_OFFSET_R12(sp)

	mfsprg2 r12
	stw r12, ISTATE_OFFSET_SP(sp)

	li r12, 0
	stw r12, ISTATE_OFFSET_LR_FRAME(sp)
	stw r12, ISTATE_OFFSET_SP_FRAME(sp)
.endm

.org 0x100
SYMBOL(exc_system_reset)
	CONTEXT_STORE

	li r3, 0
	b jump_to_kernel

.org 0x200
SYMBOL(exc_machine_check)
	CONTEXT_STORE

	li r3, 1
	b jump_to_kernel

.org 0x300
SYMBOL(exc_data_storage)
	CONTEXT_STORE

	li r3, 2
	b jump_to_kernel

.org 0x400
SYMBOL(exc_instruction_storage)
	CONTEXT_STORE

	li r3, 3
	b jump_to_kernel

.org 0x500
SYMBOL(exc_external)
	CONTEXT_STORE

	li r3, 4
	b jump_to_kernel

.org 0x600
SYMBOL(exc_alignment)
	CONTEXT_STORE

	li r3, 5
	b jump_to_kernel

.org 0x700
SYMBOL(exc_program)
	CONTEXT_STORE

	li r3, 6
	b jump_to_kernel

.org 0x800
SYMBOL(exc_fp_unavailable)
	CONTEXT_STORE

	li r3, 7
	b jump_to_kernel

.org 0x900
SYMBOL(exc_decrementer)
	CONTEXT_STORE

	li r3, 8
	b jump_to_kernel

.org 0xa00
SYMBOL(exc_reserved0)
	CONTEXT_STORE

	li r3, 9
	b jump_to_kernel

.org 0xb00
SYMBOL(exc_reserved1)
	CONTEXT_STORE

	li r3, 10
	b jump_to_kernel

.org 0xc00
SYMBOL(exc_syscall)
	CONTEXT_STORE

	b jump_to_kernel_syscall

.org 0xd00
SYMBOL(exc_trace)
	CONTEXT_STORE

	li r3, 12
	b jump_to_kernel

.org 0x1000
SYMBOL(exc_itlb_miss)
	CONTEXT_STORE

	li r3, 13
	b jump_to_kernel

.org 0x1100
SYMBOL(exc_dtlb_miss_load)
	CONTEXT_STORE

	li r3, 14
	b jump_to_kernel

.org 0x1200
SYMBOL(exc_dtlb_miss_store)
	CONTEXT_STORE

	li r3, 15
	b jump_to_kernel

.org 0x4000
jump_to_kernel:
	mfsrr1 r5
	andi. r5, r5, MSR_PR
	bne 1f
		# Previous mode was kernel.
		# We can construct a proper frame linkage.

		mfsrr0 r12
		stw r12, ISTATE_OFFSET_LR_FRAME(sp)
		mfsprg2 r12
		stw r12, ISTATE_OFFSET_SP_FRAME(sp)
	1:

	lis r12, iret@ha
	addi r12, r12, iret@l
	mtlr r12

	lis r12, exc_dispatch@ha
	addi r12, r12, exc_dispatch@l
	mtsrr0 r12

	mfmsr r12
	mfsrr1 r5
	andi. r5, r5, MSR_FP
	or r12, r12, r5		# Propagate MSR_FP from SRR1 to MSR
	ori r12, r12, (MSR_IR | MSR_DR)
	mtsrr1 r12

	addis sp, sp, 0x8000
	mr r4, sp

	rfi

jump_to_kernel_syscall:
	lis r12, syscall_handler@ha
	addi r12, r12, syscall_handler@l
	mtsrr0 r12

	lis r12, iret_syscall@ha
	addi r12, r12, iret_syscall@l
	mtlr r12

	mfsrr1 r0
	andi. r0, r0, MSR_FP
	mfmsr r12
	or r12, r12, r0		# Propagate MSR_FP from SRR1 to MSR
	ori r12, r12, (MSR_IR | MSR_DR | MSR_EE)
	mtsrr1 r12

	addis sp, sp, 0x8000
	rfi
