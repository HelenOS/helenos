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

#include <arch/boot/boot.h>
#include <arch/boot/memmapasm.h>

.section K_TEXT_START
.global kernel_image_start

KTEXT=8
KDATA=16

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
	xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %ss							# initialize stack segment register
	movl $BOOTSTRAP_OFFSET - 0x400, %esp	# initialize stack pointer
	
	call memmap_arch_init
	
	lgdt real_bootstrap_gdtr				# initialize Global Descriptor Table register
	
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0							# switch to protected mode
	
	jmpl $KTEXT, $boot_image_start
	
.code32
.align 4
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)	# checksum
	.long multiboot_header + BOOT_OFFSET
	.long unmapped_ktext_start + BOOT_OFFSET
	.long 0
	.long 0
	.long multiboot_image_start + BOOT_OFFSET
	
boot_image_start:
	movw $KDATA, %ax
	movw %ax, %es
	movw %ax, %gs
	movw %ax, %fs
	movw %ax, %ds			# kernel data + stack
	movw %ax, %ss
	
	movb $0xd1, %al			# enable A20 using the keyboard controller
	outb %al, $0x64
	movb $0xdf, %al
	outb %al, $0x60
	
	movl $BOOTSTRAP_OFFSET, %esi
	movl $BOOTSTRAP_OFFSET + BOOT_OFFSET, %edi
	movl $_hardcoded_kernel_size, %ecx
	cld
	rep movsb
	
	call map_kernel			# map kernel and turn paging on
	
	call main_bsp			# never returns

	cli
	hlt
	
multiboot_image_start:
	movl $BOOTSTRAP_OFFSET - 0x400, %esp			# initialize stack pointer
	
	lgdt protected_bootstrap_gdtr - 0x80000000		# initialize Global Descriptor Table register

	movw $KDATA, %cx
	movw %cx, %es
	movw %cx, %gs
	movw %cx, %fs
	movw %cx, %ds									# kernel data + stack
	movw %cx, %ss
	
	jmpl $KTEXT, $multiboot_meeting_point + BOOT_OFFSET
	multiboot_meeting_point:
	
	pushl %ebx										# save parameters from GRUB
	pushl %eax
	
	movl $BOOTSTRAP_OFFSET + BOOT_OFFSET, %esi
	movl $BOOTSTRAP_OFFSET, %edi
	movl $_hardcoded_unmapped_size, %ecx
	cld
	rep movsb
	
	call map_kernel									# map kernel and turn paging on
	
	popl %eax
	popl %ebx
	cmpl $MULTIBOOT_LOADER_MAGIC, %eax			# compare GRUB signature
	je valid_boot
		
		xorl %ecx, %ecx							# no memory size or map available
		movl %ecx, e801memorysize
		movl %ecx, e820counter
		
		jmp invalid_boot
		
	valid_boot:
		
		movl (%ebx), %eax						# ebx = physical address of struct multiboot_info
		
		bt $0, %eax								# mbi->flags[0] (mem_lower, mem_upper valid)
		jc mem_valid
			
			xorl %ecx, %ecx
			jmp mem_invalid
			
		mem_valid:
		movl 4(%ebx), %ecx						# mbi->mem_lower
		addl 8(%ebx), %ecx						# mbi->mem_upper
		
		mem_invalid:
		movl %ecx, e801memorysize
		
		bt $6, %eax								# mbi->flags[6] (mmap_length, mmap_addr valid)	
		jc mmap_valid
			
			xorl %edx, %edx
			jmp mmap_invalid
			
		mmap_valid:
		movl 44(%ebx), %ecx						# mbi->mmap_length
		movl 48(%ebx), %esi						# mbi->mmap_addr
		movl $e820table, %edi
		xorl %edx, %edx
		
		mmap_loop:
			cmpl $0, %ecx
			jle mmap_end
			
			movl 4(%esi), %eax					# mmap->base_addr_low
			movl %eax, (%edi)
			
			movl 8(%esi), %eax					# mmap->base_addr_high
			movl %eax, 4(%edi)
			
			movl 12(%esi), %eax					# mmap->length_low
			movl %eax, 8(%edi)
			
			movl 16(%esi), %eax					# mmap->length_high
			movl %eax, 12(%edi)
			
			movl 20(%esi), %eax					# mmap->type
			movl %eax, 16(%edi)
			
			movl (%esi), %eax					# mmap->size
			addl $0x4, %eax
			addl %eax, %esi
			subl %eax, %ecx
			addl $MEMMAP_E820_RECORD_SIZE, %edi
			incl %edx
			jmp mmap_loop
		
		mmap_end:
		
		mmap_invalid:
		movl %edx, e820counter
		
	invalid_boot:
	
	call main_bsp - BOOT_OFFSET						# never returns

	cli
	hlt

.global map_kernel
map_kernel:
	#
	# Here we setup mapping for both the unmapped and mapped sections of the kernel.
	# For simplicity, we set only one 4M page for 0x00000000 and one for 0x80000000.
	#
	movl %cr4, %ecx
	orl $(1<<4), %ecx
	movl %ecx, %cr4				# turn PSE on
	
	movl $((1<<7)|(1<<0)), %eax
	movl %eax, page_directory		# mapping 0x00000000 => 0x00000000

	movl $(page_directory+2048), %edx
	movl %eax, (%edx)			# mapping 0x80000000 => 0x00000000

	leal page_directory, %eax
	movl %eax, %cr3
	
	# turn paging on
	movl %cr0, %ebx
	orl $(1<<31), %ebx
	movl %ebx, %cr0
	ret


.section K_DATA_START

.align 4096
page_directory:
	.space 4096, 0
