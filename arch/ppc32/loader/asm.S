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
.global jump_to_kernel

halt:
	b halt

jump_to_kernel:
	
	# r3 = memmap (pa)
	# r4 = trans (pa)
	# r5 = number of kernel pages
	# r6 = real_mode (pa)
	
	mtspr srr0, r6
	
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
	# r4 = trans (pa)
	# r5 = number of kernel pages
	
	li r31, PAGE_SIZE >> 3
	li r30, 0
	
	page_copy:
		
		cmpwi r5, 0
		beq copy_end
		
		# copy single page
		
		mtctr r31
		lwz r29, 0(r4)
		
		copy_loop:
			
			lwz r28, 0(r29)
			stw r28, 0(r30)
			
			addi r29, r29, 4
			addi r30, r30, 4
			
			bdnz copy_loop
		
		subi r5, r5, 1
		addi r4, r4, 4
		b page_copy
	
	copy_end:
		
	# fill segment registers

	li r31, 16
	mtctr r31
	li r31, 0
	li r30, 0x2000
	
	seg_fill:
	
		mtsrin r30, r31
		
		addis r31, r31, 0x1000    # add 256 MB
		addi  r30, r30, 0x111     # move to next SR
		
		bdnz seg_fill
	
	# create identity mapping
	
	tlbia
	
	# start the kernel
	#
	# r3 = memmap (pa)
	
	lis r31, KERNEL_START_ADDR@ha
	addi r31, r31, KERNEL_START_ADDR@l
	
	mtspr srr0, r31
	
	mfmsr r31
	ori r31, r31, (msr_ir | msr_dr)@l
	mtspr srr1, r31
	
	rfi

.align PAGE_WIDTH
.global trans
trans:
	.space (TRANS_SIZE * TRANS_ITEM_SIZE)
