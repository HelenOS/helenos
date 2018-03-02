#
# Copyright (c) 2007 Michal Kebrt
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
#include <arch/asm/boot.h>

.text

SYMBOL(kernel_image_start)
	# initialize Stack pointer for exception modes
	mrs r4, cpsr
	bic r4, r4, #0x1f

	#FIQ Mode
	orr r3, r4, #0x11
	msr cpsr_c, r3
	ldr sp, =exc_stack

	#IRQ Mode
	orr r3, r4, #0x12
	msr cpsr_c, r3
	ldr sp, =exc_stack

	#ABORT Mode
	orr r3, r4, #0x17
	msr cpsr_c, r3
	ldr sp, =exc_stack

	#UNDEFINED Mode
	orr r3, r4, #0x1b
	msr cpsr_c, r3
	ldr sp, =exc_stack

	# switch to supervisor mode
	orr r3, r4, #0x13
	msr cpsr_c, r3

	ldr sp, =temp_stack

	bl arm32_pre_main

	#
	# Create the first stack frame.
	#
	mov fp, #0
	mov ip, sp
	push {fp, ip, lr, pc}
	sub fp, ip, #4

	bl main_bsp

	.space TEMP_STACK_SIZE
temp_stack:

	.space 1024
SYMBOL(exc_stack)

SYMBOL(supervisor_sp)
	.space 4
