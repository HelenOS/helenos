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

.section K_TEXT_START
.global kernel_image_start

.code16
#
# This is where we require any SPARTAN-kernel-compatible boot loader
# to pass control in real mode.
#
# Protected mode tables are statically initialised during compile
# time. So we can just load the respective table registers and
# switch to protected mode.
#
kernel_image_start:
	call memmap_arch_init
	cli
	xorw %ax,%ax
	movw %ax,%ds
	lgdt gdtr
	movl %cr0,%eax
	orl $0x1,%eax
	movl %eax,%cr0
	jmp 0f
0:
	movw $16,%ax
	movw %ax,%es
	movw %ax,%gs
	movw %ax,%fs
	movw %ax,%ds			# kernel data + stack
        movw %ax,%ss
	movl $0x7c00,%esp
	jmpl $8,$meeting_point
meeting_point:
.code32

	lidt idtr

	call main_bsp			# never returns
	

	cli
	hlt
