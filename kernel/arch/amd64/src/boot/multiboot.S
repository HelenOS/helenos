/*
 * Copyright (c) 2005 Ondrej Palkovsky
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2008 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <abi/asmtool.h>
#include <arch/boot/boot.h>
#include <arch/mm/page.h>
#include <arch/mm/ptl.h>
#include <arch/pm.h>
#include <genarch/multiboot/multiboot.h>
#include <arch/cpuid.h>
#include <arch/cpu.h>

#define START_STACK  (BOOT_OFFSET - BOOT_STACK_SIZE)

.section K_TEXT_START, "ax"

.code32

.macro pm_error msg
	movl \msg, %esi
	jmp pm_error_halt
.endm

.macro pm_status msg
#ifdef CONFIG_EGA
	pushl %esi
	movl \msg, %esi
	call pm_early_puts
	popl %esi
#endif
.endm

.macro pm2_status msg
#ifndef CONFIG_FB
	pm_status \msg
#endif
.endm

.align 4
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)  /* checksum */
	.long multiboot_header
	.long unmapped_ktext_start
	.long 0
	.long 0
	.long multiboot_image_start

SYMBOL(multiboot_image_start)
	cli
	cld

	/* Initialize stack pointer */
	movl $START_STACK, %esp

	/*
	 * Initialize Global Descriptor Table and
	 * Interrupt Descriptor Table registers
	 */
	lgdtl bootstrap_gdtr
	lidtl bootstrap_idtr

	/* Kernel data + stack */
	movw $GDT_SELECTOR(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %ds
	movw %cx, %ss

	/*
	 * Simics seems to remove hidden part of GS on entering user mode
	 * when _visible_ part of GS does not point to user-mode segment.
	 */
	movw $GDT_SELECTOR(UDATA_DES), %cx
	movw %cx, %fs
	movw %cx, %gs

	jmpl $GDT_SELECTOR(KTEXT32_DES), $multiboot_meeting_point
	multiboot_meeting_point:

	/*
	 * Protected 32-bit. We want to reuse the code-seg descriptor,
	 * the Default operand size must not be 1 when entering long mode.
	 */

	/* Save multiboot arguments */
	movl %eax, multiboot_eax
	movl %ebx, multiboot_ebx

	pm_status $status_prot

	movl $(INTEL_CPUID_EXTENDED), %eax
	cpuid
	cmp $(INTEL_CPUID_EXTENDED), %eax
	ja extended_cpuid_supported

		pm_error $err_extended_cpuid

	extended_cpuid_supported:

	movl $(AMD_CPUID_EXTENDED), %eax
	cpuid
	bt $(AMD_EXT_LONG_MODE), %edx
	jc long_mode_supported

		pm_error $err_long_mode

	long_mode_supported:

	bt $(AMD_EXT_NOEXECUTE), %edx
	jc noexecute_supported

		pm_error $err_noexecute

	noexecute_supported:

	movl $(INTEL_CPUID_STANDARD), %eax
	cpuid
	bt $(INTEL_FXSAVE), %edx
	jc fx_supported

		pm_error $err_fx

	fx_supported:

	bt $(INTEL_SSE2), %edx
	jc sse2_supported

		pm_error $err_sse2

	sse2_supported:

#include "vesa_prot.inc"

	pm2_status $status_prot2

	/*
	 * Enable 64-bit page translation entries - CR4.PAE = 1.
	 * Paging is not enabled until after long mode is enabled.
	 */

	movl %cr4, %eax
	orl $CR4_PAE, %eax
	movl %eax, %cr4

	/* Set up paging tables */
	leal ptl_0, %eax
	movl %eax, %cr3

	/* Enable long mode */
	movl $AMD_MSR_EFER, %ecx
	rdmsr                     /* read EFER */
	orl $AMD_LME, %eax        /* set LME = 1 */
	wrmsr

	/* Enable paging to activate long mode (set CR0.PG = 1) */
	movl %cr0, %eax
	orl $CR0_PG, %eax
	movl %eax, %cr0

	/* At this point we are in compatibility mode */
	jmpl $GDT_SELECTOR(KTEXT_DES), $start64

/** Print string to EGA display (in light red) and halt.
 *
 * Should be executed from 32 bit protected mode with paging
 * turned off. Stack is not required. This routine is used even
 * if CONFIG_EGA is not enabled. Since we are going to halt the
 * CPU anyway, it is always better to at least try to print
 * some hints.
 *
 * @param %esi Pointer to the NULL-terminated string
 *             to be print.
 *
 */
pm_error_halt:
	movl $0xb8000, %edi  /* base of EGA text mode memory */
	xorl %eax, %eax

	/* Read bits 8 - 15 of the cursor address */
	movw $0x3d4, %dx
	movb $0xe, %al
	outb %al, %dx

	movw $0x3d5, %dx
	inb %dx, %al
	shl $8, %ax

	/* Read bits 0 - 7 of the cursor address */
	movw $0x3d4, %dx
	movb $0xf, %al
	outb %al, %dx

	movw $0x3d5, %dx
	inb %dx, %al

	/* Sanity check for the cursor on screen */
	cmp $2000, %ax
	jb err_cursor_ok

		movw $1998, %ax

	err_cursor_ok:

	movw %ax, %bx
	shl $1, %eax
	addl %eax, %edi

	err_ploop:
		lodsb

		cmp $0, %al
		je err_ploop_end

		movb $0x0c, %ah  /* black background, light red foreground */
		stosw

		/* Sanity check for the cursor on the last line */
		inc %bx
		cmp $2000, %bx
		jb err_ploop

		/* Scroll the screen (24 rows) */
		movl %esi, %edx
		movl $0xb80a0, %esi
		movl $0xb8000, %edi
		movl $960, %ecx
		rep movsl

		/* Clear the 24th row */
		xorl %eax, %eax
		movl $40, %ecx
		rep stosl

		/* Go to row 24 */
		movl %edx, %esi
		movl $0xb8f00, %edi
		movw $1920, %bx

		jmp err_ploop
	err_ploop_end:

	/* Write bits 8 - 15 of the cursor address */
	movw $0x3d4, %dx
	movb $0xe, %al
	outb %al, %dx

	movw $0x3d5, %dx
	movb %bh, %al
	outb %al, %dx

	/* Write bits 0 - 7 of the cursor address */
	movw $0x3d4, %dx
	movb $0xf, %al
	outb %al, %dx

	movw $0x3d5, %dx
	movb %bl, %al
	outb %al, %dx

	cli
	hlt1:
		hlt
		jmp hlt1

/** Print string to EGA display (in light green).
 *
 * Should be called from 32 bit protected mode with paging
 * turned off. A stack space of at least 24 bytes is required,
 * but the function does not establish a stack frame.
 *
 * Macros such as pm_status and pm2_status take care that
 * this function is used only when CONFIG_EGA is enabled
 * and CONFIG_FB is disabled.
 *
 * @param %esi Pointer to the NULL-terminated string
 *             to be print.
 *
 */
pm_early_puts:
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi

	movl $0xb8000, %edi  /* base of EGA text mode memory */
	xorl %eax, %eax

	/* Read bits 8 - 15 of the cursor address */
	movw $0x3d4, %dx
	movb $0xe, %al
	outb %al, %dx

	movw $0x3d5, %dx
	inb %dx, %al
	shl $8, %ax

	/* Read bits 0 - 7 of the cursor address */
	movw $0x3d4, %dx
	movb $0xf, %al
	outb %al, %dx

	movw $0x3d5, %dx
	inb %dx, %al

	/* Sanity check for the cursor on screen */
	cmp $2000, %ax
	jb pm_puts_cursor_ok

		movw $1998, %ax

	pm_puts_cursor_ok:

	movw %ax, %bx
	shl $1, %eax
	addl %eax, %edi

	pm_puts_ploop:
		lodsb

		cmp $0, %al
		je pm_puts_ploop_end

		movb $0x0a, %ah  /* black background, light green foreground */
		stosw

		/* Sanity check for the cursor on the last line */
		inc %bx
		cmp $2000, %bx
		jb pm_puts_ploop

		/* Scroll the screen (24 rows) */
		movl %esi, %edx
		movl $0xb80a0, %esi
		movl $0xb8000, %edi
		movl $960, %ecx
		rep movsl

		/* Clear the 24th row */
		xorl %eax, %eax
		movl $40, %ecx
		rep stosl

		/* Go to row 24 */
		movl %edx, %esi
		movl $0xb8f00, %edi
		movw $1920, %bx

		jmp pm_puts_ploop
	pm_puts_ploop_end:

	/* Write bits 8 - 15 of the cursor address */
	movw $0x3d4, %dx
	movb $0xe, %al
	outb %al, %dx

	movw $0x3d5, %dx
	movb %bh, %al
	outb %al, %dx

	/* Write bits 0 - 7 of the cursor address */
	movw $0x3d4, %dx
	movb $0xf, %al
	outb %al, %dx

	movw $0x3d5, %dx
	movb %bl, %al
	outb %al, %dx

	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax

	ret

.code64

.macro long_status msg
	pushq %rdi
	movq \msg, %rdi
	call early_puts
	popq %rdi
.endm

start64:

	/*
	 * Long mode.
	 */

	movq $(PA2KA(START_STACK)), %rsp

	/* Create the first stack frame */
	pushq $0
	movq %rsp, %rbp

	long_status $status_long

	/* Call amd64_pre_main(multiboot_eax, multiboot_ebx) */
	movl multiboot_eax, %edi
	movl multiboot_ebx, %esi

#ifdef MEMORY_MODEL_large
	movabsq $amd64_pre_main, %rax
	callq *%rax
#else
	callq amd64_pre_main
#endif

	long_status $status_main

	/* Call main_bsp() */
#ifdef MEMORY_MODEL_large
	movabsq $main_bsp, %rax
	callq *%rax
#else
	callq main_bsp
#endif

	/* Not reached */
	cli
	hlt0:
		hlt
		jmp hlt0

/** Print string to EGA display.
 *
 * Should be called from long mode (with paging enabled
 * and stack established). This function is ABI compliant
 * (without red-zone).
 *
 * If CONFIG_EGA is undefined or CONFIG_FB is defined
 * then this function does nothing.
 *
 * @param %rdi Pointer to the NULL-terminated string
 *             to be printed.
 *
 */
early_puts:

#if ((defined(CONFIG_EGA)) && (!defined(CONFIG_FB)))

	/* Prologue, save preserved registers */
	pushq %rbp
	movq %rsp, %rbp
	pushq %rbx

	movq %rdi, %rsi
	movq $(PA2KA(0xb8000)), %rdi  /* base of EGA text mode memory */
	xorq %rax, %rax

	/* Read bits 8 - 15 of the cursor address */
	movw $0x3d4, %dx
	movb $0xe, %al
	outb %al, %dx

	movw $0x3d5, %dx
	inb %dx, %al
	shl $8, %ax

	/* Read bits 0 - 7 of the cursor address */
	movw $0x3d4, %dx
	movb $0xf, %al
	outb %al, %dx

	movw $0x3d5, %dx
	inb %dx, %al

	/* Sanity check for the cursor on screen */
	cmp $2000, %ax
	jb early_puts_cursor_ok

		movw $1998, %ax

	early_puts_cursor_ok:

	movw %ax, %bx
	shl $1, %rax
	addq %rax, %rdi

	early_puts_ploop:
		lodsb

		cmp $0, %al
		je early_puts_ploop_end

		movb $0x0e, %ah  /* black background, yellow foreground */
		stosw

		/* Sanity check for the cursor on the last line */
		inc %bx
		cmp $2000, %bx
		jb early_puts_ploop

		/* Scroll the screen (24 rows) */
		movq %rsi, %rdx
		movq $(PA2KA(0xb80a0)), %rsi
		movq $(PA2KA(0xb8000)), %rdi
		movl $480, %ecx
		rep movsq

		/* Clear the 24th row */
		xorl %eax, %eax
		movl $20, %ecx
		rep stosq

		/* Go to row 24 */
		movq %rdx, %rsi
		movq $(PA2KA(0xb8f00)), %rdi
		movw $1920, %bx

		jmp early_puts_ploop
	early_puts_ploop_end:

	/* Write bits 8 - 15 of the cursor address */
	movw $0x3d4, %dx
	movb $0xe, %al
	outb %al, %dx

	movw $0x3d5, %dx
	movb %bh, %al
	outb %al, %dx

	/* Write bits 0 - 7 of the cursor address */
	movw $0x3d4, %dx
	movb $0xf, %al
	outb %al, %dx

	movw $0x3d5, %dx
	movb %bl, %al
	outb %al, %dx

	/* Epilogue, restore preserved registers */
	popq %rbx
	leave

#endif

	ret

#include "vesa_real.inc"

.section K_INI_PTLS, "aw", @progbits

/** Generate initial page table contents.
 *
 * @param cnt Number of entries to generate. Must be multiple of 8.
 * @param g   Number of GB that will be added to the mapping.
 *
 */
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

/* Page table for pages in the 1st gigabyte. */
.align 4096
ptl_2_0g:
	ptl2gen 512 0

/* Page table for pages in the 2nd gigabyte. */
.align 4096
ptl_2_1g:
	ptl2gen 512 1

/* Page table for pages in the 3rd gigabyte. */
.align 4096
ptl_2_2g:
	ptl2gen 512 2

/* Page table for pages in the 4th gigabyte. */
.align 4096
ptl_2_3g:
	ptl2gen 512 3

/* Page table for pages in the 5th gigabyte. */
.align 4096
ptl_2_4g:
	ptl2gen 512 4

/* Page table for pages in the 6th gigabyte. */
.align 4096
ptl_2_5g:
	ptl2gen 512 5

/* Page table for pages in the 7th gigabyte. */
.align 4096
ptl_2_6g:
	ptl2gen 512 6

/* Page table for pages in the 8th gigabyte. */
.align 4096
ptl_2_7g:
	ptl2gen 512 7

#ifdef MEMORY_MODEL_kernel
.align 4096
ptl_1:
	/* Identity mapping for [0; 8G) */
	.quad ptl_2_0g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_1g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_2g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_3g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_4g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_5g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_6g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_7g + (PTL_WRITABLE | PTL_PRESENT)
	.fill 502, 8, 0
	/* Mapping of [0; 2G) at -2G */
	.quad ptl_2_0g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_1g + (PTL_WRITABLE | PTL_PRESENT)

.align 4096
SYMBOL(ptl_0)
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 510, 8, 0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
#endif

#ifdef MEMORY_MODEL_large
.align 4096
ptl_1:
	/* Identity mapping for [0; 8G) */
	.quad ptl_2_0g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_1g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_2g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_3g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_4g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_5g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_6g + (PTL_WRITABLE | PTL_PRESENT)
	.quad ptl_2_7g + (PTL_WRITABLE | PTL_PRESENT)
	.fill 504, 8, 0

.align 4096
SYMBOL(ptl_0)
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 255, 8, 0
	.quad ptl_1 + (PTL_WRITABLE | PTL_PRESENT)
	.fill 255, 8, 0
#endif

.section K_DATA_START, "aw", @progbits

SYMBOL(bootstrap_idtr)
	.word 0
	.long 0

SYMBOL(bootstrap_gdtr)
	.word GDT_SELECTOR(GDT_ITEMS)
	.long KA2PA(gdt)

SYMBOL(multiboot_eax)
	.long 0

SYMBOL(multiboot_ebx)
	.long 0

err_extended_cpuid:
	.asciz "Error: Extended CPUID not supported -- CPU is not 64-bit. System halted."
err_long_mode:
	.asciz "Error: 64-bit long mode not supported. System halted."
err_noexecute:
	.asciz "Error: No-execute pages not supported. System halted."
err_fx:
	.asciz "Error: FXSAVE/FXRESTORE instructions not supported. System halted."
err_sse2:
	.asciz "Error: SSE2 instructions not supported. System halted."

status_prot:
	.asciz "[prot] "
status_vesa_copy:
	.asciz "[vesa_copy] "
status_multiboot_cmdline:
	.asciz "[multiboot_cmdline] "
status_vesa_real:
	.asciz "[vesa_real] "
status_prot2:
	.asciz "[prot2] "
status_long:
	.asciz "[long] "
status_main:
	.asciz "[main] "
