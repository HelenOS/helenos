#
# Copyright (C) 2005 Ondrej Palkovsky
# Copyright (C) 2006 Martin Decky
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
#include <arch/mm/ptl.h>
#include <arch/pm.h>
#include <arch/cpu.h>
#include <arch/cpuid.h>

#define START_STACK	(BOOT_OFFSET - BOOT_STACK_SIZE)
	
.section K_TEXT_START, "ax"

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
	lgdt bootstrap_gdtr				# initialize Global Descriptor Table register

	movw $gdtselector(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %ds							# kernel data + stack
	movw %cx, %ss
	# Simics seems to remove hidden part of GS on entering user mode
	#  when _visible_ part of GS does not point to user-mode segment
	movw $gdtselector(UDATA_DES), %cx
	movw %cx, %fs
	movw %cx, %gs
	
	jmpl $gdtselector(KTEXT32_DES), $multiboot_meeting_point
	multiboot_meeting_point:
	
	movl %eax, grub_eax							# save parameters from GRUB
	movl %ebx, grub_ebx
	
	# Protected 32-bit. We want to reuse the code-seg descriptor,
	# the Default operand size must not be 1 when entering long mode
	
	movl $0x80000000, %eax  
 	cpuid
 	cmp $0x80000000, %eax						# any function > 80000000h?
	jbe long_mode_unsupported
	movl $(AMD_CPUID_EXTENDED), %eax			# Extended function code 80000001
	cpuid
	bt $29, %edx								# Test if long mode is supported.
 	jc long_mode_supported

	long_mode_unsupported:
		cli
		hlt
	
	long_mode_supported:
	
	# Enable 64-bit page transaltion entries - CR4.PAE = 1.
	# Paging is not enabled until after long mode is enabled
	
	movl %cr4, %eax
	btsl $5, %eax
	movl %eax, %cr4

	# Set up paging tables
	
	leal ptl_0, %eax
	movl %eax, %cr3
	
	# Enable long mode
	
	movl $EFER_MSR_NUM, %ecx	# EFER MSR number
	rdmsr						# Read EFER
	btsl $AMD_LME_FLAG, %eax	# Set LME=1
	wrmsr						# Write EFER
	
	# Enable paging to activate long mode (set CR0.PG=1)
	
	movl %cr0, %eax
	btsl $31, %eax
	movl %eax, %cr0
	
	# At this point we are in compatibility mode
	
	jmpl $gdtselector(KTEXT_DES), $start64

.code64
start64:
	movq $(PA2KA(START_STACK)), %rsp
	movl grub_eax, %eax
	movl grub_ebx, %ebx
	
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
			movl %ecx, init
			jmp mods_end
		
		mods_valid:
		
		xorq %rcx, %rcx
		movl 20(%ebx), %ecx						# mbi->mods_count
		movl %ecx, init
		
		cmpl $0, %ecx
		je mods_end
		
		movl 24(%ebx), %esi						# mbi->mods_addr
		movq $init, %rdi
		
		mods_loop:
			
			xorq %rdx, %rdx
			movl 0(%esi), %edx					# mods->mod_start
			addq $0xffffffff80000000, %rdx
			movq %rdx, 4(%rdi)
			
			xorq %rdx, %rdx
			movl 4(%esi), %edx
			subl 0(%esi), %edx					# mods->mod_end - mods->mod_start
			movq %rdx, 12(%rdi)
			
			addl $16, %esi
			addq $16, %rdi
			
			loop mods_loop
			
		mods_end:
		
		bt $6, %eax								# mbi->flags[6] (mmap_length, mmap_addr valid)	
		jc mmap_valid
			
			xorl %edx, %edx
			jmp mmap_invalid
			
		mmap_valid:
		movl 44(%ebx), %ecx						# mbi->mmap_length
		movl 48(%ebx), %esi						# mbi->mmap_addr
		movq $e820table, %rdi
		xorl %edx, %edx
		
		mmap_loop:
			cmpl $0, %ecx
			jle mmap_end
			
			movl 4(%esi), %eax					# mmap->base_addr_low
			movl %eax, (%rdi)
			
			movl 8(%esi), %eax					# mmap->base_addr_high
			movl %eax, 4(%rdi)
			
			movl 12(%esi), %eax					# mmap->length_low
			movl %eax, 8(%rdi)
			
			movl 16(%esi), %eax					# mmap->length_high
			movl %eax, 12(%rdi)
			
			movl 20(%esi), %eax					# mmap->type
			movl %eax, 16(%rdi)
			
			movl (%esi), %eax					# mmap->size
			addl $0x4, %eax
			addl %eax, %esi
			subl %eax, %ecx
			addq $MEMMAP_E820_RECORD_SIZE, %rdi
			incl %edx
			jmp mmap_loop
		
		mmap_end:
		
		mmap_invalid:
		movl %edx, e820counter
		
	invalid_boot:
	
#ifdef CONFIG_SMP
	
	# copy AP bootstrap routines below 1 MB
	
	movq $BOOT_OFFSET, %rsi
	movq $AP_BOOT_OFFSET, %rdi
	movq $_hardcoded_unmapped_size, %rcx
	cld
	rep movsb
	
#endif
	
	call main_bsp   # never returns
	
	cli
	hlt
				
.section K_DATA_START, "aw", @progbits
.align 4096

# Identical mapping of first 64MB and the same of -2GB -> 0	
.global ptl_2
ptl_2:	
	.quad 0x0 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x200000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x400000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x600000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x800000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0xa00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0xc00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0xe00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1000000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1200000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1400000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1600000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1800000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1a00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1c00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x1e00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2000000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2200000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2400000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2600000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2800000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2a00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2c00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x2e00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3000000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3200000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3400000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3600000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3800000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3a00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3c00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad 0x3e00000 | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	
.align 4096
.global ptl_1
ptl_1:
	.quad ptl_2 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 509,8,0
	.quad ptl_2 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 1,8,0
	
.align 4096
.global ptl_0
ptl_0:
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 510,8,0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)

.global bootstrap_gdtr
bootstrap_gdtr:
	.word gdtselector(GDT_ITEMS)
	.long KA2PA(gdt)

grub_eax:
	.long 0

grub_ebx:
	.long 0
