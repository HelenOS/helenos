/*
 * Copyright (c) 2005 Martin Decky
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

#include <arch/asm/regname.h>
#include <arch/msr.h>

.text

.global userspace_asm
.global iret
.global iret_syscall
.global memcpy_from_uspace
.global memcpy_to_uspace
.global memcpy_from_uspace_failover_address
.global memcpy_to_uspace_failover_address
.global early_putchar

userspace_asm:
	
	/*
	 * r3 = uspace_uarg
	 * r4 = stack
	 * r5 = entry
	 */
	
	/* Disable interrupts */
	
	mfmsr r31
	rlwinm r31, r31, 0, 17, 15
	mtmsr r31
	
	/* Set entry point */
	
	mtsrr0 r5
	
	/* Set problem state, enable interrupts */
	
	ori r31, r31, MSR_PR
	ori r31, r31, MSR_EE
	mtsrr1 r31
	
	/* Set stack */
	
	mr sp, r4
	
	/* %r6 is defined to hold pcb_ptr - set it to 0 */
	
	xor r6, r6, r6
	
	/* Jump to userspace */
	
	rfi

iret:
	
	/* Disable interrupts */
	
	mfmsr r31
	rlwinm r31, r31, 0, 17, 15
	mtmsr r31
	
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

iret_syscall:
	
	/* Reset decrementer */
	
	li r31, 1000
	mtdec r31
	
	/* Disable interrupts */
	
	mfmsr r31
	rlwinm r31, r31, 0, 17, 15
	mtmsr r31
	
	lwz r0, 8(sp)
	lwz r2, 12(sp)
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

memcpy_from_uspace:
memcpy_to_uspace:
	
	srwi. r7, r5, 3
	addi r6, r3, -4
	addi r4, r4, -4
	beq 2f
	
	andi. r0, r6, 3
	mtctr r7
	bne 5f
	
	1:
	
		lwz r7, 4(r4)
		lwzu r8, 8(r4)
		stw r7, 4(r6)
		stwu r8, 8(r6)
		bdnz 1b
		
		andi. r5, r5, 7
	
	2:
	
		cmplwi 0, r5, 4
		blt 3f
		
		lwzu r0, 4(r4)
		addi r5, r5, -4
		stwu r0, 4(r6)
	
	3:
	
		cmpwi 0, r5, 0
		beqlr
		mtctr r5
		addi r4, r4, 3
		addi r6, r6, 3
	
	4:
	
		lbzu r0, 1(r4)
		stbu r0, 1(r6)
		bdnz 4b
		blr
	
	5:
	
		subfic r0, r0, 4
		mtctr r0
	
	6:
	
		lbz r7, 4(r4)
		addi r4, r4, 1
		stb r7, 4(r6)
		addi r6, r6, 1
		bdnz 6b
		subf r5, r0, r5
		rlwinm. r7, r5, 32-3, 3, 31
		beq 2b
		mtctr r7
		b 1b

memcpy_from_uspace_failover_address:
memcpy_to_uspace_failover_address:
	/* Return zero, failure */
	xor r3, r3, r3
	blr

early_putchar:
	blr
