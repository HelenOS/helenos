#
# Copyright (C) 2001-2004 Jakub Jermar
# Copyright (C) 2005-2006 Martin Decky
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

KTEXT=8
KDATA=16

.code32
.align 4
.global multiboot_image_start
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)	# checksum
	.long multiboot_header
	.long unmapped_ktext_start
	.long 0
	.long 0
	.long multiboot_image_start
	
multiboot_image_start:
	movl $START_STACK, %esp			# initialize stack pointer
	lgdt KA2PA(bootstrap_gdtr)		# initialize Global Descriptor Table register

	movw $KDATA, %cx
	movw %cx, %es
	movw %cx, %gs
	movw %cx, %fs
	movw %cx, %ds					# kernel data + stack
	movw %cx, %ss
	
	jmpl $KTEXT, $multiboot_meeting_point
	multiboot_meeting_point:
	
	pushl %ebx							# save parameters from GRUB
	pushl %eax
	
	call map_kernel							# map kernel and turn paging on
	
	popl %eax
	popl %ebx
	cmpl $MULTIBOOT_LOADER_MAGIC, %eax				# compare GRUB signature
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
		
		bt $3, %eax								# mbi->flags[3] (mods_count, mods_addr valid)	
		jc mods_valid
			
			xorl %ecx, %ecx
			xorl %edx, %edx
			jmp mods_invalid
		
		mods_valid:
		movl 20(%ebx), %ecx						# mbi->mods_count
		cmpl $0, %ecx
		je mods_invalid
		
		movl 24(%ebx), %esi						# mbi->mods_addr
		movl 0(%esi), %edx						# mods->mod_start
		movl 4(%esi), %ecx						# mods->mod_end
		subl %edx, %ecx
		addl $0x80000000, %edx
		
		mods_invalid:
		movl %ecx, init_size
		movl %edx, init_addr
		
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
	
#ifdef CONFIG_SMP
	
	# copy AP bootstrap routines below 1 MB
	
	movl $BOOT_OFFSET, %esi
	movl $AP_BOOT_OFFSET, %edi
	movl $_hardcoded_unmapped_size, %ecx
	cld
	rep movsb
	
#endif
	
	call main_bsp								# never returns

	cli
	hlt

.global map_kernel
map_kernel:
	#
	# Here we setup mapping for both the unmapped and mapped sections of the kernel.
	# For simplicity, we map the entire 4G space.
	#
	movl %cr4, %ecx
	orl $(1<<4), %ecx
	movl %ecx, %cr4							# turn PSE on
	
	movl $(page_directory+0), %esi
	movl $(page_directory+2048), %edi
	xorl %ecx, %ecx
	xorl %ebx, %ebx
0:
	movl $((1<<7)|(1<<0)), %eax
	orl %ebx, %eax
	movl %eax, (%esi,%ecx,4)					# mapping 0x00000000+%ecx*4M => 0x00000000+%ecx*4M
	movl %eax, (%edi,%ecx,4)					# mapping 0x80000000+%ecx*4M => 0x00000000+%ecx*4M
	addl $(4*1024*1024), %ebx

	incl %ecx
	cmpl $512, %ecx
	jl 0b

	movl %esi, %cr3
	
	# turn paging on
	movl %cr0, %ebx
	orl $(1<<31), %ebx
	movl %ebx, %cr0
	ret


.section K_DATA_START, "aw", @progbits

.align 4096
page_directory:
	.space 4096, 0
