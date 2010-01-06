#
# Copyright (c) 2005 Ondrej Palkovsky
# Copyright (c) 2006 Martin Decky
# Copyright (c) 2008 Jakub Jermar
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
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)  # checksum
	.long multiboot_header
	.long unmapped_ktext_start
	.long 0
	.long 0
	.long multiboot_image_start

multiboot_image_start:
	cld
	movl $START_STACK, %esp             # initialize stack pointer
	lgdtl bootstrap_gdtr                # initialize Global Descriptor Table register
	
	movw $gdtselector(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %ds                       # kernel data + stack
	movw %cx, %ss
	
	#
	# Simics seems to remove hidden part of GS on entering user mode
	# when _visible_ part of GS does not point to user-mode segment.
	#
	
	movw $gdtselector(UDATA_DES), %cx
	movw %cx, %fs
	movw %cx, %gs
	
	jmpl $gdtselector(KTEXT32_DES), $multiboot_meeting_point
	multiboot_meeting_point:
	
	movl %eax, grub_eax                 # save parameters from GRUB
	movl %ebx, grub_ebx
	
	#
	# Protected 32-bit. We want to reuse the code-seg descriptor,
	# the Default operand size must not be 1 when entering long mode.
	#
	
	movl $(INTEL_CPUID_EXTENDED), %eax
	cpuid
	cmp $(INTEL_CPUID_EXTENDED), %eax
	ja extended_cpuid_supported
	
		movl $extended_cpuid_msg, %esi
		jmp error_halt
	
	extended_cpuid_supported:
	
	movl $(AMD_CPUID_EXTENDED), %eax
	cpuid
	bt $(AMD_EXT_LONG_MODE), %edx
	jc long_mode_supported
	
		movl $long_mode_msg, %esi
		jmp error_halt
	
	long_mode_supported:
	
	bt $(AMD_EXT_NOEXECUTE), %edx
	jc noexecute_supported
	
		movl $noexecute_msg, %esi
		jmp error_halt
	
	noexecute_supported:
	
	movl $(INTEL_CPUID_STANDARD), %eax
	cpuid
	bt $(INTEL_FXSAVE), %edx
	jc fx_supported
	
		movl $fx_msg, %esi
		jmp error_halt
	
	fx_supported:
	
	bt $(INTEL_SSE2), %edx
	jc sse2_supported
	
		movl $sse2_msg, %esi
		jmp error_halt
	
	sse2_supported:

#include "vesa_prot.inc"

	#
	# Enable 64-bit page translation entries - CR4.PAE = 1.
	# Paging is not enabled until after long mode is enabled.
	#
	
	movl %cr4, %eax
	btsl $5, %eax
	movl %eax, %cr4
	
	# set up paging tables
	
	leal ptl_0, %eax
	movl %eax, %cr3
	
	# enable long mode
	
	movl $EFER_MSR_NUM, %ecx            # EFER MSR number
	rdmsr                               # read EFER
	btsl $AMD_LME_FLAG, %eax            # set LME = 1
	wrmsr                               # write EFER
	
	# enable paging to activate long mode (set CR0.PG = 1)
	
	movl %cr0, %eax
	btsl $31, %eax
	movl %eax, %cr0
	
	# at this point we are in compatibility mode
	
	jmpl $gdtselector(KTEXT_DES), $start64

.code64
start64:
	movq $(PA2KA(START_STACK)), %rsp
	
	# call arch_pre_main(grub_eax, grub_ebx)
	xorq %rdi, %rdi
	movl grub_eax, %edi
	xorq %rsi, %rsi
	movl grub_ebx, %esi
	call arch_pre_main
	
	call main_bsp
	
	# not reached
	
	cli
	hlt0:
		hlt
		jmp hlt0

# Print string from %esi to EGA display (in red) and halt
error_halt:
	movl $0xb8000, %edi       # base of EGA text mode memory
	xorl %eax, %eax
	
	movw $0x3d4, %dx          # read bits 8 - 15 of the cursor address
	movb $0xe, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	inb %dx, %al
	shl $8, %ax
	
	movw $0x3d4, %dx          # read bits 0 - 7 of the cursor address
	movb $0xf, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	inb %dx, %al
	
	cmp $1920, %ax
	jbe cursor_ok
	
		movw $1920, %ax       # sanity check for the cursor on the last line
	
	cursor_ok:
	
	movw %ax, %bx
	shl $1, %eax
	addl %eax, %edi
	
	movw $0x0c00, %ax         # black background, light red foreground
	
	ploop:
		lodsb
		cmp $0, %al
		je ploop_end
		stosw
		inc %bx
		jmp ploop
	ploop_end:
	
	movw $0x3d4, %dx          # write bits 8 - 15 of the cursor address
	movb $0xe, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	movb %bh, %al
	outb %al, %dx
	
	movw $0x3d4, %dx          # write bits 0 - 7 of the cursor address
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

.section K_INI_PTLS, "aw", @progbits

#
# Macro for generating initial page table contents.
# @param cnt Number of entries to generat. Must be multiple of 8.
# @param g   Number of GB that will be added to the mapping.
#
.macro ptl2gen cnt g 
.if \cnt
	ptl2gen "\cnt - 8" \g 
	.quad ((\cnt - 8) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad ((\cnt - 7) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad ((\cnt - 6) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad ((\cnt - 5) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad ((\cnt - 4) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad ((\cnt - 3) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad ((\cnt - 2) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
	.quad ((\cnt - 1) * 0x200000) + (\g * 1024 * 1024 * 1024) | (PTL_WRITABLE | PTL_PRESENT | PTL_2MB_PAGE)
.endif
.endm

# Page table for pages in the first gigabyte.
.align 4096
.global ptl_2_0g
ptl_2_0g:	
	ptl2gen 512 0

# Page table for pages in the second gigabyte.
.align 4096
.global ptl_2_1g
ptl_2_1g:
	ptl2gen 512 1

# Page table for pages in the third gigabyte.
.align 4096
.global ptl_2_2g
ptl_2_2g:
	ptl2gen 512 2

# Page table for pages in the fourth gigabyte.
.align 4096
.global ptl_2_3g
ptl_2_3g:
	ptl2gen 512 3

.align 4096
.global ptl_1
ptl_1:
	# Identity mapping for [0; 4G)
	.quad ptl_2_0g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_1g + (PTL_WRITABLE | PTL_PRESENT) 
	.quad ptl_2_2g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_3g + (PTL_WRITABLE | PTL_PRESENT)
	.fill 506, 8, 0
	# Mapping of [0; 1G) at -2G
	.quad ptl_2_0g + (PTL_WRITABLE | PTL_PRESENT)
	.fill 1, 8, 0

.align 4096
.global ptl_0
ptl_0:
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 255,8,0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 254,8,0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)

.section K_DATA_START, "aw", @progbits

.global bootstrap_gdtr
bootstrap_gdtr:
	.word gdtselector(GDT_ITEMS)
	.long KA2PA(gdt)

grub_eax:
	.long 0

grub_ebx:
	.long 0

extended_cpuid_msg:
	.asciz "Error: Extended CPUID not supported -- CPU is not 64-bit. System halted."
long_mode_msg:
	.asciz "Error: 64-bit long mode not supported. System halted."
noexecute_msg:
	.asciz "Error: No-execute pages not supported. System halted."
fx_msg:
	.asciz "Error: FXSAVE/FXRESTORE instructions not supported. System halted."
sse2_msg:
	.asciz "Error: SSE2 instructions not supported. System halted."
