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

#include <arch/asm/regname.h>
#include <arch/mm/page.h>

.section K_UNMAPPED_TEXT_START, "ax"

.macro CONTEXT_STORE
	
	# save R12 in SPRG1, backup CR in R12
	# save SP in SPRG2

	mtsprg1 r12
	mfcr r12
	mtsprg2 sp
	
	# check whether SP is in kernel
	
	andis. sp, sp, 0x8000
	bne 1f
	
		# stack is in user-space
		
		mfsprg0 sp
	
	b 2f
	
	1:
	
		# stack is in kernel
		
		mfsprg2 sp
		subis sp, sp, 0x8000
	
	2:
	
	subi sp, sp, 164
	stw r0, 8(sp)
	stw r2, 12(sp)
	stw r3, 16(sp)
	stw r4, 20(sp)
	stw r5, 24(sp)
	stw r6, 28(sp)
	stw r7, 32(sp)
	stw r8, 36(sp)
	stw r9, 40(sp)
	stw r10, 44(sp)
	stw r11, 48(sp)
	stw r13, 52(sp)
	stw r14, 56(sp)
	stw r15, 60(sp)
	stw r16, 64(sp)
	stw r17, 68(sp)
	stw r18, 72(sp)
	stw r19, 76(sp)
	stw r20, 80(sp)
	stw r21, 84(sp)
	stw r22, 88(sp)
	stw r23, 92(sp)
	stw r24, 96(sp)
	stw r25, 100(sp)
	stw r26, 104(sp)
	stw r27, 108(sp)
	stw r28, 112(sp)
	stw r29, 116(sp)
	stw r30, 120(sp)
	stw r31, 124(sp)
	
	stw r12, 128(sp)
	
	mfsrr0 r12
	stw r12, 132(sp)
	
	mfsrr1 r12
	stw r12, 136(sp)
	
	mflr r12
	stw r12, 140(sp)
	
	mfctr r12
	stw r12, 144(sp)
	
	mfxer r12
	stw r12, 148(sp)
	
	mfdar r12
	stw r12, 152(sp)
	
	mfsprg1 r12
	stw r12, 156(sp)
	
	mfsprg2 r12
	stw r12, 160(sp)
.endm

.org 0x100
.global exc_system_reset
exc_system_reset:
	CONTEXT_STORE
	
	li r3, 0
	b jump_to_kernel

.org 0x200
.global exc_machine_check
exc_machine_check:
	CONTEXT_STORE
	
	li r3, 1
	b jump_to_kernel

.org 0x300
.global exc_data_storage
exc_data_storage:
	CONTEXT_STORE
	
	b data_storage

.org 0x400
.global exc_instruction_storage
exc_instruction_storage:
	CONTEXT_STORE
	
	b instruction_storage

.org 0x500
.global exc_external
exc_external:
	CONTEXT_STORE
	
	li r3, 4
	b jump_to_kernel

.org 0x600
.global exc_alignment
exc_alignment:
	CONTEXT_STORE
	
	li r3, 5
	b jump_to_kernel

.org 0x700
.global exc_program
exc_program:
	CONTEXT_STORE
	
	li r3, 6
	b jump_to_kernel

.org 0x800
.global exc_fp_unavailable
exc_fp_unavailable:
	CONTEXT_STORE
	
	li r3, 7
	b jump_to_kernel

.org 0x900
.global exc_decrementer
exc_decrementer:
	CONTEXT_STORE
	
	li r3, 8
	b jump_to_kernel

.org 0xa00
.global exc_reserved0
exc_reserved0:
	CONTEXT_STORE
	
	li r3, 9
	b jump_to_kernel

.org 0xb00
.global exc_reserved1
exc_reserved1:
	CONTEXT_STORE
	
	li r3, 10
	b jump_to_kernel

.org 0xc00
.global exc_syscall
exc_syscall:
	CONTEXT_STORE	
	
	b jump_to_kernel_syscall

.org 0xd00
.global exc_trace
exc_trace:
	CONTEXT_STORE
	
	li r3, 12
	b jump_to_kernel

.org 0x4000
data_storage:
	li r3, 2
	mr r4, sp
	addi r4, r4, 8
	bl pht_refill_real
	
	cmpwi r3, 0
	bne iret_real
	
	li r3, 2
	b jump_to_kernel

instruction_storage:
	li r3, 3
	mr r4, sp
	addi r4, r4, 8
	bl pht_refill_real
	
	cmpwi r3, 0
	bne iret_real
	
	li r3, 3
	b jump_to_kernel

jump_to_kernel:
	lis r12, iret@ha
	addi r12, r12, iret@l
	mtlr r12
	
	lis r12, exc_dispatch@ha
	addi r12, r12, exc_dispatch@l
	mtsrr0 r12
	
	mfmsr r12
	ori r12, r12, (msr_ir | msr_dr)@l
	mtsrr1 r12
	
	addis sp, sp, 0x8000
	mr r4, sp
	addi r4, r4, 8
	
	rfi

jump_to_kernel_syscall:
	lis r12, syscall_handler@ha
	addi r12, r12, syscall_handler@l
	mtsrr0 r12
	
	lis r12, iret_syscall@ha
	addi r12, r12, iret_syscall@l
	mtlr r12
	
	mfmsr r12
	ori r12, r12, (msr_ir | msr_dr)@l
	mtsrr1 r12
	
	addis sp, sp, 0x8000
	rfi

iret_real:
	lwz r0, 8(sp)
	lwz r2, 12(sp)
	lwz r3, 16(sp)
	lwz r4, 20(sp)
	lwz r5, 24(sp)
	lwz r6, 28(sp)
	lwz r7, 32(sp)
	lwz r8, 36(sp)
	lwz r9, 40(sp)
	lwz r10, 44(sp)
	lwz r11, 48(sp)
	lwz r13, 52(sp)
	lwz r14, 56(sp)
	lwz r15, 60(sp)
	lwz r16, 64(sp)
	lwz r17, 68(sp)
	lwz r18, 72(sp)
	lwz r19, 76(sp)
	lwz r20, 80(sp)
	lwz r21, 84(sp)
	lwz r22, 88(sp)
	lwz r23, 92(sp)
	lwz r24, 96(sp)
	lwz r25, 100(sp)
	lwz r26, 104(sp)
	lwz r27, 108(sp)
	lwz r28, 112(sp)
	lwz r29, 116(sp)
	lwz r30, 120(sp)
	lwz r31, 124(sp)
	
	lwz r12, 128(sp)
	mtcr r12
	
	lwz r12, 132(sp)
	mtsrr0 r12
	
	lwz r12, 136(sp)
	mtsrr1 r12
	
	lwz r12, 140(sp)
	mtlr r12
	
	lwz r12, 144(sp)
	mtctr r12
	
	lwz r12, 148(sp)
	mtxer r12
	
	lwz r12, 156(sp)
	lwz sp, 160(sp)
	
	rfi
