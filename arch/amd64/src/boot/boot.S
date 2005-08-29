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
	cli
	xorw %ax,%ax
	movw %ax,%ds
	movw %ax,%ss            # initialize stack segment register
	movl $0x7c00,%esp	# initialize stack pointer
	
	call memmap_arch_init
	
	mov $0x80000000, %eax  
	cpuid
	cmp $0x80000000, %eax 	# any function > 80000000h?
	jbe no_long_mode
	mov $0x80000001, %eax	# Extended function code 80000001
	cpuid
	bt $29, %edx 		# Test if long mode is supported.
	jnc no_long_mode

# Fill out GDTR.base, IDTR.base
	leal gdtr, %eax
	movl gdt_addr, %ebx
	movl %ebx, 2(%eax)

	movl idt_addr, %ebx
	leal idtr, %eax
	movl %ebx, 2(%eax)

# Load gdtr, idtr	
	lgdt gdtr
	lidt idtr
	
	mov $1, %eax    # Enable protected mode (CR0.PE = 1)
	mov %eax, %cr0 

	jmpl $8, $now_in_prot
	
now_in_prot:
	

no_long_mode:
1:
	jmp 1b
			

.section K_DATA_START	
.align 4096
page_directory:
	.space 4096, 0

gdt_addr:	
	.quad gdt + 0x80000000
idt_addr:	
	.quad idt + 0x80000000
