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

.section K_TEXT_START, "ax"

.global kernel_image_start
kernel_image_start:

	# load temporary stack
	
	lis sp, end_stack@ha
	addi sp, sp, end_stack@l
	
	# r3 contains physical address of bootinfo_t
	# r4 contains size of bootinfo_t
	
	lis r31, 0x80000000@ha
	addi r31, r31, 0x80000000@l
	
	add r3, r3, r31

	lis r31, bootinfo@ha
	addi r31, r31, bootinfo@l  # r31 = bootinfo
	
	cmpwi r4, 0
	beq bootinfo_end
	
	bootinfo_loop:
		
		lwz r30, 0(r3)
		stw r30, 0(r31)
		
		addi r3, r3, 4
		addi r31, r31, 4
		subi r4, r4, 4
		
		cmpwi r4, 0
		bgt bootinfo_loop
		
	bootinfo_end:
	
	b main_bsp

.section K_DATA_START, "aw", @progbits

	.space TEMP_STACK_SIZE
end_stack:
