
# Copyright (c) 2005 Ondrej Palkovsky
# Copyright (c) 2006 Martin Decky
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
	
#ifdef CONFIG_FB
	mov $vesa_init, %esi
	mov $VESA_INIT_SEGMENT << 4, %edi
	mov $e_vesa_init - vesa_init, %ecx
	cld
	rep movsb

	mov $VESA_INIT_SEGMENT << 4, %edi
	jmpl *%edi
	
	vesa_meeting_point:
	
	mov %esi, KA2PA(vesa_ph_addr)
	mov %di, KA2PA(vesa_height)
	shr $16, %edi
	mov %di, KA2PA(vesa_width)
	mov %bx, KA2PA(vesa_scanline)
	shr $16, %ebx
	mov %bx, KA2PA(vesa_bpp)
#endif	
	
	# Enable 64-bit page translation entries - CR4.PAE = 1.
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
	btsl $AMD_LME_FLAG, %eax	# Set LME = 1
	wrmsr						# Write EFER
	
	# Enable paging to activate long mode (set CR0.PG = 1)
	
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
			
			xorq %rcx, %rcx
			movq %rcx, init
			jmp mods_end
		
		mods_valid:
		
		xorq %rcx, %rcx
		movl 20(%ebx), %ecx						# mbi->mods_count
		movq %rcx, init
		
		cmpl $0, %ecx
		je mods_end
		
		movl 24(%ebx), %esi						# mbi->mods_addr
		movq $init, %rdi
		
		mods_loop:
			
			xorq %rdx, %rdx
			movl 0(%esi), %edx					# mods->mod_start
			movq $0xffff800000000000, %r10
			addq %r10, %rdx
			movq %rdx, 8(%rdi)
			
			xorq %rdx, %rdx
			movl 4(%esi), %edx
			subl 0(%esi), %edx					# mods->mod_end - mods->mod_start
			movq %rdx, 16(%rdi)
			
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

#ifdef CONFIG_FB
.code32
vesa_init:
	jmp $gdtselector(VESA_INIT_DES), $vesa_init_real - vesa_init
	
.code16
vesa_init_real:
	
	mov %cr0, %eax
	and $~1, %eax
	mov %eax, %cr0
	
	jmp $VESA_INIT_SEGMENT, $vesa_init_real2 - vesa_init
	
vesa_init_real2:
	
	mov $VESA_INIT_SEGMENT, %bx
	
	mov %bx, %es
	mov %bx, %fs
	mov %bx, %gs
	mov %bx, %ds
	mov %bx, %ss
	
	movl $0x0000fffc, %esp
    movl $0x0000fffc, %ebp
	
#define VESA_INFO_SIZE 1024

#define VESA_MODE_ATTRIBUTES_OFFSET 0
#define VESA_MODE_LIST_PTR_OFFSET 14
#define VESA_MODE_SCANLINE_OFFSET 16
#define VESA_MODE_WIDTH_OFFSET 18
#define VESA_MODE_HEIGHT_OFFSET 20
#define VESA_MODE_BPP_OFFSET 25
#define VESA_MODE_PHADDR_OFFSET 40

#define VESA_END_OF_MODES 0xffff

#define VESA_OK 0x4f

#define VESA_GET_INFO 0x4f00
#define VESA_GET_MODE_INFO 0x4f01
#define VESA_SET_MODE 0x4f02
#define VESA_SET_PALETTE 0x4f09

#define CONFIG_VESA_BPP_a 255

#if CONFIG_VESA_BPP == 24
#define CONFIG_VESA_BPP_VARIANT 32
#endif

	mov $VESA_GET_INFO, %ax
	mov $e_vesa_init - vesa_init, %di
	push %di
	int $0x10
	
	pop %di
	cmp $VESA_OK, %al
	jnz 0f
	
	mov 2 + VESA_MODE_LIST_PTR_OFFSET(%di), %si
	mov %si, %gs
	mov VESA_MODE_LIST_PTR_OFFSET(%di), %si
	
	add $VESA_INFO_SIZE, %di

1:# Try next mode
	mov %gs:(%si), %cx
	cmp $VESA_END_OF_MODES, %cx
	jz 0f
	
	inc %si
	inc %si
	push %cx
	push %di
	push %si
	mov $VESA_GET_MODE_INFO, %ax
	int $0x10
	
	pop %si
	pop %di
	pop %cx
	cmp $VESA_OK, %al
	jnz 0f
	
	mov $CONFIG_VESA_WIDTH, %ax
	cmp VESA_MODE_WIDTH_OFFSET(%di), %ax
	jnz 1b
	
	mov $CONFIG_VESA_HEIGHT, %ax
	cmp VESA_MODE_HEIGHT_OFFSET(%di), %ax
	jnz 1b
	
	mov $CONFIG_VESA_BPP, %al
	cmp VESA_MODE_BPP_OFFSET(%di), %al

#ifdef CONFIG_VESA_BPP_VARIANT
	jz 2f
	
	mov $CONFIG_VESA_BPP_VARIANT, %al
	cmp VESA_MODE_BPP_OFFSET(%di), %al
#endif
	jnz 1b
	
2:
	
	mov %cx, %bx
	or $0xc000, %bx
	push %di
	mov $VESA_SET_MODE, %ax
	int $0x10
	
	pop %di
	cmp $VESA_OK, %al
	jnz 0f

#if CONFIG_VESA_BPP == 8
	
	# Set 3:2:3 VGA palette
	
	mov VESA_MODE_ATTRIBUTES_OFFSET(%di), %ax
	push %di
	mov $vga323 - vesa_init, %di
	mov $0x100, %ecx
	
	bt $5, %ax							# Test if VGA compatible registers are present
	jnc vga_compat
		
		# Try VESA routine to set palette
		
		mov $VESA_SET_PALETTE, %ax
		xor %bl, %bl
		xor %dx, %dx
		int $0x10
		
		jmp vga_not_compat
	
	vga_compat:
		
		# Try VGA registers to set palette
		
		movw $0x3c6, %dx						# Set palette mask
		movb $0xff, %al
		outb %al, %dx
		
		movw $0x3c8, %dx						# First index to set
		xor %al, %al
		outb %al, %dx
		
		movw $0x3c9, %dx						# Data port
		vga_loop:
			movb %es:2(%di), %al
			outb %al, %dx
			
			movb %es:1(%di), %al
			outb %al, %dx
			
			movb %es:(%di), %al
			outb %al, %dx
			
			addw $4, %di
			loop vga_loop
		
	vga_not_compat:
	
	pop %di
	
#endif
	
	mov VESA_MODE_PHADDR_OFFSET(%di), %esi
	mov VESA_MODE_WIDTH_OFFSET(%di), %ax
	shl $16, %eax
	mov VESA_MODE_HEIGHT_OFFSET(%di), %ax
	mov VESA_MODE_BPP_OFFSET(%di), %bl
	xor %bh, %bh
	shl $16, %ebx
	mov VESA_MODE_SCANLINE_OFFSET(%di), %bx
	mov %eax, %edi
	
8:
	
	mov %cr0, %eax
	or $1, %eax
	mov %eax, %cr0
	
	jmp 9f
9:
	
	ljmpl $gdtselector(KTEXT32_DES), $(vesa_init_protect - vesa_init + VESA_INIT_SEGMENT << 4)
	
0:# No prefered mode found
	mov $0x111, %cx
	push %di
	push %cx
	mov $VESA_GET_MODE_INFO, %ax
	int $0x10
	
	pop %cx
	pop %di
	cmp $VESA_OK, %al
	jnz 1f
	jz 2b						# Force relative jump

1:
	mov $0x0003, %ax
	int $0x10
	mov $0xffffffff, %edi		# EGA text mode used, because of problems with VESA
	xor %ax, %ax
	jz 8b						# Force relative jump

vga323:
#include "vga323.pal"	
	
.code32
vesa_init_protect:
	movw $gdtselector(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %ds							# kernel data + stack
	movw %cx, %ss
	# Simics seems to remove hidden part of GS on entering user mode
	#  when _visible_ part of GS does not point to user-mode segment
	movw $gdtselector(UDATA_DES), %cx
	movw %cx, %fs
	movw %cx, %gs
	
	movl $START_STACK, %esp			# initialize stack pointer
	
	jmpl $gdtselector(KTEXT32_DES), $vesa_meeting_point
	
.align 4
e_vesa_init:
#endif

# Print string from %esi to EGA display (in red) and halt
error_halt:
	movl $0xb8000, %edi						# base of EGA text mode memory
	xorl %eax, %eax
	
	movw $0x3d4, %dx						# read bits 8 - 15 of the cursor address
	movb $0xe, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	inb %dx, %al
	shl $8, %ax
	
	movw $0x3d4, %dx						# read bits 0 - 7 of the cursor address
	movb $0xf, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	inb %dx, %al
	
	cmp $1920, %ax
	jbe cursor_ok
		movw $1920, %ax						# sanity check for the cursor on the last line
	cursor_ok:
	
	movw %ax, %bx
	shl $1, %eax
	addl %eax, %edi
	
	movw $0x0c00, %ax						# black background, light red foreground
	cld
	
	ploop:
		lodsb
		cmp $0, %al
		je ploop_end
		stosw
		inc %bx	
		jmp ploop
	ploop_end:
	
	movw $0x3d4, %dx						# write bits 8 - 15 of the cursor address
	movb $0xe, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	movb %bh, %al
	outb %al, %dx
	
	movw $0x3d4, %dx						# write bits 0 - 7 of the cursor address
	movb $0xf, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	movb %bl, %al
	outb %al, %dx
		
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
	.fill 255,8,0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 254,8,0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)

.global bootstrap_gdtr
bootstrap_gdtr:
	.word gdtselector(GDT_ITEMS)
	.long KA2PA(gdt)

grub_eax:
	.long 0

grub_ebx:
	.long 0

extended_cpuid_msg:
	.asciz "Extended CPUID not supported. System halted."
long_mode_msg:
	.asciz "64 bit long mode not supported. System halted."
noexecute_msg:
	.asciz "No-execute pages not supported. System halted."
fx_msg:
	.asciz "FXSAVE/FXRESTORE instructions not supported. System halted."
sse2_msg:
	.asciz "SSE2 instructions not supported. System halted."
