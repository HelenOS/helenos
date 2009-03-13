#
# Copyright (c) 2001-2004 Jakub Jermar
# Copyright (c) 2005-2006 Martin Decky
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

#include <arch/boot/boot.h>
#include <arch/boot/memmap.h>
#include <arch/mm/page.h>
#include <arch/pm.h>

#define START_STACK (BOOT_OFFSET - BOOT_STACK_SIZE)

.section K_TEXT_START, "ax"

.code32
.align 4
.global multiboot_image_start
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)  # checksum
	.long multiboot_header
	.long unmapped_ktext_start
	.long 0
	.long 0
	.long multiboot_image_start

multiboot_image_start:
	cld
	movl $START_STACK, %esp     # initialize stack pointer
	lgdt KA2PA(bootstrap_gdtr)  # initialize Global Descriptor Table register
	
	movw $gdtselector(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %fs
	movw %cx, %gs
	movw %cx, %ds               # kernel data + stack
	movw %cx, %ss
	
	jmpl $gdtselector(KTEXT_DES), $multiboot_meeting_point
	multiboot_meeting_point:
	
	movl %eax, grub_eax         # save parameters from GRUB
	movl %ebx, grub_ebx
	
	xorl %eax, %eax
	cpuid
	cmp $0x0, %eax              # any function > 0?
	jbe pse_unsupported
	movl $0x1, %eax             # basic function code 1
	cpuid
	bt $3, %edx                 # test if PSE is supported
	jc pse_supported
	
	pse_unsupported:
		movl $pse_msg, %esi
		jmp error_halt
	
	pse_supported:

#include "vesa_prot.inc"

	# map kernel and turn paging on
	call map_kernel
	
	# call arch_pre_main(grub_eax, grub_ebx)
	pushl grub_ebx
	pushl grub_eax
	call arch_pre_main
	
	call main_bsp
	
	# not reached
	cli
	hlt0:
		hlt
		jmp hlt0

.global map_kernel
map_kernel:
	#
	# Here we setup mapping for both the unmapped and mapped sections of the kernel.
	# For simplicity, we map the entire 4G space.
	#
	movl %cr4, %ecx
	orl $(1 << 4), %ecx                 # turn PSE on
	andl $(~(1 << 5)), %ecx             # turn PAE off
	movl %ecx, %cr4
	
	movl $(page_directory + 0), %esi
	movl $(page_directory + 2048), %edi
	xorl %ecx, %ecx
	xorl %ebx, %ebx
	
	floop:
		movl $((1 << 7) | (1 << 1) | (1 << 0)), %eax
		orl %ebx, %eax
		movl %eax, (%esi, %ecx, 4)      # mapping 0x00000000 + %ecx * 4M => 0x00000000 + %ecx * 4M
		movl %eax, (%edi, %ecx, 4)      # mapping 0x80000000 + %ecx * 4M => 0x00000000 + %ecx * 4M
		addl $(4 * 1024 * 1024), %ebx
		
		incl %ecx
		cmpl $512, %ecx
		jl floop
	
	movl %esi, %cr3
	
	movl %cr0, %ebx
	orl $(1 << 31), %ebx                # turn paging on
	movl %ebx, %cr0
	ret

# Print string from %esi to EGA display (in red) and halt
error_halt:
	movl $0xb8000, %edi         # base of EGA text mode memory
	xorl %eax, %eax
	
	movw $0x3d4, %dx            # read bits 8 - 15 of the cursor address
	movb $0xe, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	inb %dx, %al
	shl $8, %ax
	
	movw $0x3d4, %dx            # read bits 0 - 7 of the cursor address
	movb $0xf, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	inb %dx, %al
	
	cmp $1920, %ax
	jbe cursor_ok
	
		movw $1920, %ax         # sanity check for the cursor on the last line
	
	cursor_ok:
	
	movw %ax, %bx
	shl $1, %eax
	addl %eax, %edi
	
	movw $0x0c00, %ax           # black background, light red foreground
	
	ploop:
		lodsb
		cmp $0, %al
		je ploop_end
		stosw
		inc %bx
		jmp ploop
	ploop_end:
	
	movw $0x3d4, %dx            # write bits 8 - 15 of the cursor address
	movb $0xe, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	movb %bh, %al
	outb %al, %dx
	
	movw $0x3d4, %dx            # write bits 0 - 7 of the cursor address
	movb $0xf, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	movb %bl, %al
	outb %al, %dx
	
	cli
	hlt1:
		hlt
		jmp hlt1

#include "vesa_real.inc"

.section K_DATA_START, "aw", @progbits

.align 4096
page_directory:
	.space 4096, 0

grub_eax:
	.long 0

grub_ebx:
	.long 0

pse_msg:
	.asciz "Page Size Extension not supported. System halted."
