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

#include "asm.h"
#include "regname.h"

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
	# r6 = bytes to copy
	# r7 = real_mode (pa)
	# r8 = framebuffer (pa)
	# r9 = scanline
	
	# disable interrupts
	
	mfmsr r31
	rlwinm r31, r31, 0, 17, 15
	mtmsr r31
	
	# set real_mode meeting point address
	
	mtspr srr0, r7
	
	# jumps to real_mode
	
	mfmsr r31
	lis r30, ~0@h
	ori r30, r30, ~(msr_ir | msr_dr | msr_ee)@l
	and r31, r31, r30
	mtspr srr1, r31
	
	sync
	isync
	rfi

.section REALMODE, "ax"
.align PAGE_WIDTH
.global real_mode

real_mode:
	
	# copy kernel to proper location
	#
	# r5 = trans (pa)
	# r6 = bytes to copy
	# r8 = framebuffer (pa)
	# r9 = scanline
	
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
	
	# initially fill segment registers
	
	li r31, 0
	
	li r29, 8
	mtctr r29
	li r30, 0                     # ASID 0 (VSIDs 0 .. 7)

	seg_fill_uspace:
	
		mtsrin r30, r31
		addi r30, r30, 1
		addis r31, r31, 0x1000    # move to next SR
		
		bdnz seg_fill_uspace
	
	li r29, 8
	mtctr r29
	lis r30, 0x4000               # priviledged access only
	ori r30, r30, 8               # ASID 0 (VSIDs 8 .. 15)
	
	seg_fill_kernel:
	
		mtsrin r30, r31
		addi r30, r30, 1
		addis r31, r31, 0x1000    # move to next SR
		
		bdnz seg_fill_kernel
	
	# invalidate block address translation registers
	
	li r30, 0
	
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
	
	# create empty Page Hash Table
	# on top of memory, size 64 KB
	
	lwz r31, 0(r3)                # r31 = memory size
	
	lis r30, 65536@h
	ori r30, r30, 65536@l         # r30 = 65536
	
	subi r29, r30, 1              # r29 = 65535
	
	sub r31, r31, r30
	andc r31, r31, r29            # pht = ALIGN_DOWN(memory_size - 65536, 65536)
	
	mtsdr1 r31
	
	li r29, 2
	srw r30, r30, r29             # r30 = 16384
	li r29, 0
	
	pht_clear:
		
		# write zeroes
		
		stw r29, 0(r31)
		
		addi r31, r31, 4
		subi r30, r30, 4
		
		cmpwi r30, 0
		beq clear_end
		
		bdnz pht_clear
		
	clear_end:
	
#ifdef CONFIG_BAT
	
	# create BAT identity mapping
	
	lwz r31, 0(r3)                # r31 = memory size
	
	lis r29, 0x0002
	cmpw r31, r29
	blt no_bat                    # less than 128 KB -> no BAT
	
	li r29, 18
	srw r31, r31, r29             # r31 = total >> 18
	
	# create Block Length mask by replicating
	# the leading logical one 14 times
	
	li r29, 14
	mtctr r31
	li r29, 1
	
	bat_mask:
		srw r30, r31, r29         # r30 = mask >> 1
		or r31, r31, r30          # mask = mask | r30
		
		bdnz bat_mask
	
	andi. r31, r31, 0x07ff        # mask = mask & 0x07ff (BAT can map up to 256 MB)
	
	li r29, 2
	slw r31, r31, r29             # mask = mask << 2
	ori r31, r31, 0x0002          # mask = mask | 0x0002 (priviledged access only)
	
	lis r29, 0x8000
	or r29, r29, r31
	
	lis r30, 0x0000
	ori r30, r30, 0x0002
	
	mtspr ibat0u, r29
	mtspr ibat0l, r30
	
	mtspr dbat0u, r29
	mtspr dbat0l, r30
	
	no_bat:
	
#endif
	
	tlbsync
	
	# start the kernel
	#
	# pc = KERNEL_START_ADDR
	# r3 = bootinfo (pa)
	# sprg0 = KA2PA(KERNEL_START_ADDR)
	# sprg3 = physical memory size
	# sp = 0 (pa)
	
	lis r31, KERNEL_START_ADDR@ha
	addi r31, r31, KERNEL_START_ADDR@l
	
	mtspr srr0, r31
	
	subis r31, r31, 0x8000
	mtsprg0 r31
	
	lwz r31, 0(r3)
	mtsprg3 r31
	
	li sp, 0
	
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
