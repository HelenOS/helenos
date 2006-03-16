#
# Copyright (C) 2005 Jakub Jermar
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
#include <arch/boot/boot.h>
#include <arch/boot/memmap.h>

.section K_TEXT_START, "ax"

.global kernel_image_start
kernel_image_start:

	# load temporary stack
	
	lis sp, end_stack@ha
	addi sp, sp, end_stack@l
	
	# r10 contains physical address to memmap_t
	# from boot loader
	
	lis r31, 0x80000000@ha
	addi r31, r31, 0x80000000@l
	
	add r3, r3, r31

	lis r31, memmap@ha
	addi r31, r31, memmap@l    # r31 = memmap
	
	lwz r30, 0(r3)             # memmap->total
	stw r30, 0(r31)
	
	lwzu r30, 4(r3)            # memmap->count
	stwu r30, 4(r31)
	
	cmpwi r30, 0
	beq memmap_end
	
	mtctr r30
	memmap_loop:
	
		lwzu r30, 4(r3)           # memmap->zones[i].start
		stwu r30, 4(r31)
		
		lwzu r30, 4(r3)           # memmap->zones[i].size
		stwu r30, 4(r31)
	
	bdnz memmap_loop
	memmap_end:
	
	b main_bsp

.section K_DATA_START, "aw", @progbits

	.space TEMP_STACK_SIZE
end_stack:
