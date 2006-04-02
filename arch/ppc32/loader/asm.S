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

#include "asm.h"
#include "regname.h"

.data

flush_buffer:
	.space (L1_CACHE_LINES * L1_CACHE_BYTES)

.text

.global halt
.global memcpy
.global jump_to_kernel

halt:
	b halt

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


jump_to_kernel:
	
	# r3 = bootinfo (pa)
	# r4 = bootinfo_size
	# r5 = trans (pa)
	# r6 = kernel size
	# r7 = framebuffer (pa)
	# r8 = real_mode (pa)
	
	mtspr srr0, r8
	
	# jumps to real_mode
	
	mfmsr r31
	lis r30, ~0@h
	ori r30, r30, ~(msr_ir | msr_dr)@l
	and r31, r31, r30
	mtspr srr1, r31
	rfi

.section REALMODE
.align PAGE_WIDTH
.global real_mode

real_mode:
	
	# copy kernel to proper location
	#
	# r5 = trans (pa)
	# r6 = kernel size
	# r7 = framebuffer (pa)
	
	li r31, PAGE_SIZE >> 2
	li r30, 0
	
	page_copy:
		
		cmpwi r6, 0
		beq copy_end
		
		# copy page
		
		mtctr r31
		lwz r29, 0(r5)
		
		copy_loop:
			
			lwz r28, 0(r29)
			stw r28, 0(r30)
			
			addi r29, r29, 4
			addi r30, r30, 4
			subi r6, r6, 4
			
			cmpwi r6, 0
			beq copy_end
			
			bdnz copy_loop
		
		addi r5, r5, 4
		b page_copy
	
	copy_end:
	
	# invalidate segment registers

	li r31, 16
	mtctr r31
	li r31, 0
	li r30, 0

	seg_fill:
	
		mtsrin r30, r31
		addis r31, r31, 0x1000    # move to next SR
		
		bdnz seg_fill
	
	# invalidate block address translation registers
	
	mtspr ibat0u, r30
	mtspr ibat0l, r30
	
	mtspr ibat1u, r30
	mtspr ibat1l, r30
	
	mtspr ibat2u, r30
	mtspr ibat2l, r30
	
	mtspr ibat3u, r30
	mtspr ibat3l, r30
	
	mtspr dbat0u, r30
	mtspr dbat0l, r30
	
	mtspr dbat1u, r30
	mtspr dbat1l, r30
	
	mtspr dbat2u, r30
	mtspr dbat2l, r30
	
	mtspr dbat3u, r30
	mtspr dbat3l, r30
	
	# create identity mapping
	
	# FIXME: map exactly the size of RAM
	
	lis r31, 0x8000
	ori r31, r31, 0x0ffe
	
	lis r30, 0x0000
	ori r30, r30, 0x0002
	
	mtspr ibat0u, r31
	mtspr ibat0l, r30
	
	mtspr dbat0u, r31
	mtspr dbat0l, r30
	
	# FIXME: temporal framebuffer mapping
	
	lis r31, 0xf000
	ori r31, r31, 0x0ffe
	
	mr r30, r7
	ori r30, r30, 0x0002
	
	mtspr dbat1u, r31
	mtspr dbat1l, r30
	
	tlbia
	
	# start the kernel
	#
	# r3 = bootinfo (pa)
	
	lis r31, KERNEL_START_ADDR@ha
	addi r31, r31, KERNEL_START_ADDR@l
	
	mtspr srr0, r31
	
	mfmsr r31
	ori r31, r31, (msr_ir | msr_dr)@l
	mtspr srr1, r31
	
	sync
	isync
	rfi

.align PAGE_WIDTH
.global trans
trans:
	.space (TRANS_SIZE * TRANS_ITEM_SIZE)
