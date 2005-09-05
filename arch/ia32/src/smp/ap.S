#
# Copyright (C) 2001-2004 Jakub Jermar
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

#
# Init code for application processors.
#

.section K_TEXT_START_2

#ifdef __SMP__

.global ap_boot

KTEXT=8
KDATA=16

# This piece of code is real-mode and is meant to be alligned at 4K boundary.
# The requirement for such an alignment comes from MP Specification's STARTUP IPI
# requirements.

.align 4096
ap_boot:
.code16
	cli
	xorw %ax, %ax
	movw %ax, %ds

	lgdt real_bootstrap_gdtr	# initialize Global Descriptor Table register
	
	movl %cr0, %eax
	orl $1, %eax
	movl %eax, %cr0				# switch to protected mode
	jmpl $KTEXT, $jump_to_kernel
	
jump_to_kernel:
.code32
	movw $KDATA, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movl $(ctx-0x80000000), %eax	# KA2PA((__address) &ctx)
	movl (%eax), %esp
	subl $0x80000000, %esp			# KA2PA(ctx.sp)

	call map_kernel					# map kernel and turn paging on
	
	addl $0x80000000, %esp			# PA2KA(ctx.sp)
	
	jmpl $KTEXT, $main_ap

#endif /* __SMP__ */
