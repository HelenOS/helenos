#
# Copyright (C) 2001-2004 Ondrej Palkovsky
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

#define __ASM__

#include <arch/mm/page.h>	
#include <arch/mm/ptl.h>
#include <arch/pm.h>
#include <arch/cpu.h>
#include <arch/cpuid.h>
#include <arch/boot/boot.h>

#define START_STACK_64  0xffffffff80007c00
					
#
# This is where we require any SPARTAN-kernel-compatible boot loader
# to pass control in real mode.
#
# Protected mode tables are statically initialised during compile
# time. So we can just load the respective table registers and
# switch to protected mode.
#

#define START_STACK (BOOTSTRAP_OFFSET-0x400)
	
.section K_TEXT_START
.code16
.global kernel_image_start
kernel_image_start:
	cli
	xorw %ax,%ax
	movw %ax,%ds
	movw %ax,%ss            # initialize stack segment register
	movl $(START_STACK), %esp	# initialize stack pointer
	
	call memmap_arch_init
	
	movl $0x80000000, %eax  
	cpuid
	cmp $0x80000000, %eax 	# any function > 80000000h?
	jbe no_long_mode
	movl $(AMD_CPUID_EXTENDED), %eax # Extended function code 80000001
	cpuid
	bt $29, %edx 		# Test if long mode is supported.
	jnc no_long_mode

	# Load gdtr, idtr
	lgdt bsp_bootstrap_gdtr
	
	movl %cr0,%eax
	orl $0x1,%eax
	movl %eax,%cr0			# switch to protected mode

	jmpl $gdtselector(KTEXT32_DES), $now_in_prot

no_long_mode:
1:
	jmp 1b

# Protected 16-bit. We want to reuse the code-seg descriptor,
# the Default operand size must not be 1 when entering long mode	
now_in_prot:  
	# Set up stack & data descriptors
	movw $gdtselector(KDATA_DES), %ax
	movw %ax, %ds
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	movb $0xd1, %al		# enable A20 using the keyboard controller
	outb %al, $0x64
	movb $0xdf, %al
	outb %al, $0x60

	
	# Enable 64-bit page transaltion entries - CR4.PAE = 1.
	# Paging is not enabled until after long mode is enabled
	movl %cr4, %eax
	btsl $5, %eax
	movl %eax, %cr4

	# Set up paging tables
	leal ptl_0, %eax
	movl %eax, %cr3
	
	# Enable long mode
	movl $EFER_MSR_NUM, %ecx   # EFER MSR number
	rdmsr                   # Read EFER
	btsl $AMD_LME_FLAG, %eax            # Set LME=1
	wrmsr                   # Write EFER
	
	# Enable paging to activate long mode (set CR0.PG=1)
	movl %cr0, %eax
	btsl $31, %eax
	movl %eax, %cr0
	
	# At this point we are in compatibility mode
	jmpl $gdtselector(KTEXT_DES), $start64

.code64
start64:
	movq $(PA2KA(START_STACK)), %rsp

	# Copy kernel to higher physical memory
	movq $BOOTSTRAP_OFFSET, %rsi
	movq $BOOTSTRAP_OFFSET + BOOT_OFFSET, %rdi
	movq $_hardcoded_kernel_size, %rcx
	cld
	rep movsb
	
	call main_bsp   # never returns
1:
	jmp 1b
				
.section K_DATA_START
.align 4096

# Identical mapping of first 16MB and the same of -2GB -> 0	
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
	
.align 4096
.global ptl_1
ptl_1:
	.quad ptl_2 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 509,8,0
	.quad ptl_2 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 2,8,0
	
.align 4096
.global ptl_0
ptl_0:
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 510,8,0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)

.global gdtr_inst				
bsp_bootstrap_gdtr:
	.word gdtselector(GDT_ITEMS)
	.long KA2PA(gdt)-BOOT_OFFSET

.global ap_bootstrap_gdtr
ap_bootstrap_gdtr:
	.word gdtselector(GDT_ITEMS)
	.long KA2PA(gdt)-BOOT_OFFSET
