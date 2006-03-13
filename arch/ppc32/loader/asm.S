#
# Copyright (C) 2006 Martin Decky
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

#include "regname.h"
#include "spr.h"

.data

flush_buffer:
	.space (L1_CACHE_LINES * L1_CACHE_BYTES)

.text

.global memsetb
.global memcpy
.global jump_to_kernel

memsetb:
	rlwimi r5, r5, 8, 16, 23
	rlwimi r5, r5, 16, 0, 15
	
	addi r14, r3, -4
	
	cmplwi 0, r4, 4
	blt 7f
	
	stwu r5, 4(r14)
	beqlr
	
	andi. r15, r14, 3
	add r4, r15, r4
	subf r14, r15, r14
	srwi r15, r4, 2
	mtctr r15
	
	bdz 6f
	
	1:
		stwu r5, 4(r14)
		bdnz 1b
	
	6:
	
	andi. r4, r4, 3
	
	7:
	
	cmpwi 0, r4, 0
	beqlr
	
	mtctr r4
	addi r6, r6, 3
	
	8:
	
	stbu r5, 1(r14)
	bdnz 8b
	
	blr

memcpy:
	srwi. r7, r5, 3
	addi r6, r3, -4
	addi r4, r4, -4
	beq	2f
	
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
	
flush_instruction_cache:

	# Flush data cache
	
	lis r3, flush_buffer@h
	ori r3, r3, flush_buffer@l
	li r4, L1_CACHE_LINES
	mtctr r4
	
	0:
	
	lwz r4, 0(r3)
	addi r3, r3, L1_CACHE_BYTES
	bdnz 0b
	
	# Invalidate instruction cache
	
	li r3, 0
	ori	r3, r3, (HID0_ICE | HID0_DCE | HID0_ICFI | HID0_DCI)
	mfspr r4, SPRN_HID0
	or r5, r4, r3
	isync
	mtspr SPRN_HID0, r5
	sync
	isync
	
	# Enable instruction cache
	
	ori	r5, r4, HID0_ICE
	mtspr SPRN_HID0, r5
	sync
	isync
	blr

jump_to_kernel:
	mtspr SPRN_SRR0, r3
	mfmsr r3
	lis r4, ~0@h
	ori r4, r4, ~(MSR_IR | MSR_DR)@l
	and r3, r3, r4
	mtspr SPRN_SRR1, r3
	bl flush_instruction_cache
	rfi
