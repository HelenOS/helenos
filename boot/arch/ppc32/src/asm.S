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
#include <arch/arch.h>
#include <arch/regname.h>

.macro SMC_COHERENCY addr
	dcbst 0, \addr
	sync
	icbi 0, \addr
	sync
	isync
.endm

.macro FLUSH_DCACHE addr
	dcbst 0, \addr
	sync
	isync
.endm

.macro TLB_FLUSH reg
	li \reg, 0
	sync

	.rept 64
		tlbie \reg
		addi \reg, \reg, 0x1000
	.endr

	eieio
	tlbsync
	sync
.endm

.macro BAT_COMPUTE base size mask lower upper
	# less than 128 KB -> no BAT

	lis \upper, 0x0002
	cmpw \size, \upper
	blt no_bat

	# mask = total >> 18

	li \upper, 18
	srw \mask, \size, \upper

	# create Block Length mask by replicating
	# the leading logical one 14 times

	li \upper, 14
	mtctr \mask
	li \upper, 1

	0:
		# mask = (mask >> 1) | mask

		srw \lower, \mask, \upper
		or \mask, \mask, \lower

		bdnz 0b

	# mask = mask & 0x07ff
	# (BAT can map up to 256 MB)

	andi. \mask, \mask, 0x07ff

	# mask = (mask << 2) | 0x0002
	# (priviledged access only)

	li \upper, 2
	slw \mask, \mask, \upper
	ori \mask, \mask, 0x0002

	lis \upper, (0x8000 + \base)
	or \upper, \upper, \mask

	lis \lower, \base
	ori \lower, \lower, 0x0002
.endm

.section BOOTSTRAP, "ax"

SYMBOL(start)
	lis r4, ofw_cif@ha
	addi r4, r4, ofw_cif@l
	stw r5, 0(r4)

	bl ofw_init
	b bootstrap

.text

FUNCTION_BEGIN(halt)
	b halt
FUNCTION_END(halt)

FUNCTION_BEGIN(jump_to_kernel)
	# arguments:
	# r3 = bootinfo (physical address)
	# r4 = translate table (physical address)
	# r5 = pages to translate
	# r6 = real mode meeting point (physical address)

	# disable interrupts

	mfmsr r31
	rlwinm r31, r31, 0, 17, 15
	mtmsr r31

	# set real mode meeting point physical address

	mtspr srr0, r6

	# jump to real_mode

	mfmsr r31
	lis r30, ~0@h
	ori r30, r30, ~(msr_ir | msr_dr | msr_ee)@l
	and r31, r31, r30
	mtspr srr1, r31

	sync
	isync
	rfi
FUNCTION_END(jump_to_kernel)

.section REALMODE, "ax"

.align PAGE_WIDTH
SYMBOL(real_mode)

	# arguments:
	# r3 = bootinfo (physical address)
	# r4 = translate table (physical address)
	# r5 = pages to translate

	# move the images of components to the proper
	# location using the translate table

	li r31, PAGE_SIZE >> 2
	li r30, 0

	page_copy:

		cmpwi r5, 0
		beq copy_end

		mtctr r31
		lwz r29, 0(r4)

		copy_loop:

			lwz r28, 0(r29)
			stw r28, 0(r30)

			SMC_COHERENCY r30

			addi r29, r29, 4
			addi r30, r30, 4

			bdnz copy_loop

		addi r4, r4, 4
		subi r5, r5, 1
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

	lwz r31, 4(r3)                # r31 = memory size

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
		FLUSH_DCACHE r31

		addi r31, r31, 4
		subi r30, r30, 4

		cmpwi r30, 0
		beq clear_end

		bdnz pht_clear

	clear_end:

	# create BAT identity mapping

	lwz r31, 4(r3)                # r31 = memory size

	lis r30, 268435456@h
	ori r30, r30, 268435456@l     # r30 = 256 MB

	# BAT0

	# r29 = min(r31, r30)

	cmpw r31, r30
	blt bat0_r31

		mr r29, r30
		b bat0_r30

	bat0_r31:

		mr r29, r31

	bat0_r30:

	BAT_COMPUTE 0x0000 r29 r28 r27 r26
	mtspr ibat0u, r26
	mtspr ibat0l, r27

	mtspr dbat0u, r26
	mtspr dbat0l, r27

	# BAT1

	sub r31, r31, r29             # r31 = r31 - r29

	# r29 = min(r31, r30)

	cmpw r31, r30
	blt bat1_r31

		mr r29, r30
		b bat1_r30

	bat1_r31:

		mr r29, r31

	bat1_r30:

	BAT_COMPUTE 0x1000 r29 r28 r27 r26
	mtspr ibat1u, r26
	mtspr ibat1l, r27

	mtspr dbat1u, r26
	mtspr dbat1l, r27

	# BAT2

	sub r31, r31, r29             # r31 = r31 - r29

	# r29 = min(r31, r30)

	cmpw r31, r30
	blt bat2_r31

		mr r29, r30
		b bat2_r30

	bat2_r31:

		mr r29, r31

	bat2_r30:

	BAT_COMPUTE 0x2000 r29 r28 r27 r26
	mtspr ibat2u, r26
	mtspr ibat2l, r27

	mtspr dbat2u, r26
	mtspr dbat2l, r27

	# BAT3

	sub r31, r31, r29             # r31 = r31 - r29

	# r29 = min(r31, r30)

	cmpw r31, r30
	blt bat3_r31

		mr r29, r30
		b bat3_r30

	bat3_r31:

		mr r29, r31

	bat3_r30:

	BAT_COMPUTE 0x3000 r29 r28 r27 r26
	mtspr ibat3u, r26
	mtspr ibat3l, r27

	mtspr dbat3u, r26
	mtspr dbat3l, r27

	no_bat:

	# flush TLB

	TLB_FLUSH r31

	# start the kernel
	#
	# pc = PA2KA(BOOT_OFFSET)
	# r3 = bootinfo (physical address)
	# sprg0 = BOOT_OFFSET
	# sprg3 = physical memory size
	# sp = 0 (enforces the usage of sprg0 as exception stack)

	lis r31, PA2KA(BOOT_OFFSET)@ha
	addi r31, r31, PA2KA(BOOT_OFFSET)@l
	mtspr srr0, r31

	lis r31, BOOT_OFFSET@ha
	addi r31, r31, BOOT_OFFSET@l
	mtsprg0 r31

	# bootinfo starts with a 64 bit integer containing
	# the physical memory size, get the lower 4 bytes

	lwz r31, 4(r3)
	mtsprg3 r31

	li sp, 0

	mfmsr r31
	ori r31, r31, (msr_ir | msr_dr)@l
	mtspr srr1, r31

	sync
	isync
	rfi
