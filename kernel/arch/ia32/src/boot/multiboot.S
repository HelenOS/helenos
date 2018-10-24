/*
 * Copyright (c) 2001 Jakub Jermar
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2011 Martin Sucha
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
#include <arch/boot/memmap.h>
#include <arch/mm/page.h>
#include <arch/pm.h>
#include <genarch/multiboot/multiboot.h>
#include <arch/cpuid.h>
#include <arch/cpu.h>

// TODO: most of this file can be rewritten in C

// TODO: FB state should be checked dynamically from provided multiboot info.
//       Currently we only enable EGA statically, which forces us to rebuild
//       the image to get very early debug output.

.section .multiboot, "a"

.align 4
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
#ifdef CONFIG_FB
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)  /* checksum */
#else
	.long MULTIBOOT_HEADER_FLAGS_NOFB
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS_NOFB)  /* checksum */
#endif
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
#ifdef CONFIG_FB
	.long 0
	.long CONFIG_BFB_WIDTH
	.long CONFIG_BFB_HEIGHT
	.long CONFIG_BFB_BPP
#endif

.section K_TEXT_START, "ax"

.code32

.macro pm_status msg
#if defined(CONFIG_EGA) && !defined(CONFIG_FB)
	pushl %esi
	movl \msg, %esi
	call pm_early_puts
	popl %esi
#endif
.endm

.macro pm2_status msg
	pushl \msg
	call early_puts
.endm

SYMBOL(multiboot_image_start)
	cli
	cld

	/* Initialize stack pointer */
	movl $bootstack_top, %esp

	/*
	 * Initialize Global Descriptor Table and
	 * Interrupt Descriptor Table registers
	 */
	lgdtl bootstrap_gdtr
	lidtl bootstrap_idtr

	/* Kernel data + stack */
	movw $GDT_SELECTOR(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %fs
	movw %cx, %gs
	movw %cx, %ds
	movw %cx, %ss

	jmpl $GDT_SELECTOR(KTEXT_DES), $multiboot_meeting_point
	multiboot_meeting_point:

	/* Save multiboot arguments */
	movl %eax, multiboot_eax
	movl %ebx, multiboot_ebx

	pm_status $status_prot

#ifndef PROCESSOR_i486

	pm_status $status_prot2

	movl $(INTEL_CPUID_LEVEL), %eax
	cpuid
	cmp $0x0, %eax  /* any function > 0? */
	jbe pse_unsupported

	movl $(INTEL_CPUID_STANDARD), %eax
	cpuid
	bt $(INTEL_PSE), %edx
	jnc pse_unsupported

		/* Map kernel and turn paging on */
		pm_status $status_pse
		call map_kernel_pse
		jmp stack_init

#endif /* PROCESSOR_i486 */

	pse_unsupported:

		/* Map kernel and turn paging on */
		pm_status $status_non_pse
		call map_kernel_non_pse

	stack_init:
	movl $PA2KA(bootstack_top), %esp

	/* Create the first stack frame */
	pushl $0
	movl %esp, %ebp

	pm2_status $status_prot3

	/* Call ia32_pre_main(multiboot_eax, multiboot_ebx) */
	pushl multiboot_ebx
	pushl multiboot_eax
	call ia32_pre_main

	pm2_status $status_main

	/* Call main_bsp() */
	call main_bsp

	/* Not reached */
	cli
	hlt0:
		hlt
		jmp hlt0

/** Setup mapping for the kernel (PSE variant)
 *
 * Setup mapping for both the unmapped and mapped sections
 * of the kernel. For simplicity, we map the entire 4G space.
 *
 */
FUNCTION_BEGIN(map_kernel_pse)
	/* Paging features */
	movl %cr4, %ecx
	orl $CR4_PSE, %ecx	/* PSE on */
	andl $~CR4_PAE, %ecx	/* PAE off */
	movl %ecx, %cr4

	movl $(page_directory + 0), %esi
	movl $(page_directory + 2048), %edi
	xorl %ecx, %ecx
	xorl %ebx, %ebx

	floop_pse:
		movl $(PDE_4M | PDE_RW | PDE_P), %eax
		orl %ebx, %eax
		/* Mapping 0x00000000 + %ecx * 4M => 0x00000000 + %ecx * 4M */
		movl %eax, (%esi, %ecx, 4)
		/* Mapping 0x80000000 + %ecx * 4M => 0x00000000 + %ecx * 4M */
		movl %eax, (%edi, %ecx, 4)
		addl $(4 * 1024 * 1024), %ebx

		incl %ecx
		cmpl $512, %ecx
		jl floop_pse

	movl %esi, %cr3

	movl %cr0, %ebx
	orl $CR0_PG, %ebx	/* paging on */
	movl %ebx, %cr0
	ret
FUNCTION_END(map_kernel_pse)

/** Setup mapping for the kernel (non-PSE variant).
 *
 * Setup mapping for both the unmapped and mapped sections
 * of the kernel. For simplicity, we map the entire 4G space.
 *
 */
FUNCTION_BEGIN(map_kernel_non_pse)
	/* Paging features */
	movl %cr4, %ecx
	andl $~CR4_PAE, %ecx  /* PAE off */
	movl %ecx, %cr4

	/* Allocate space for page tables */
	// TODO: we shouldn't need to put this allocation in ballocs,
	//       since it's static.
	movl $KA2PA(ballocs), %edi
	movl $page_tables, 0(%edi)
	movl $(2 * 1024 * 1024), 4(%edi)

	/* Fill page tables */
	xorl %ecx, %ecx
	xorl %ebx, %ebx
	mov $page_tables, %esi

	floop_pt:
		movl $(PTE_RW | PTE_P), %eax
		orl %ebx, %eax
		movl %eax, (%esi, %ecx, 4)
		addl $PAGE_SIZE, %ebx

		incl %ecx
		cmpl $(512 * 1024), %ecx

		jl floop_pt

	/* Fill page directory */
	movl $(page_directory + 0), %esi
	movl $(page_directory + 2048), %edi
	xorl %ecx, %ecx
	movl $page_tables, %ebx

	floop:
		movl $(PDE_RW | PDE_P), %eax
		orl %ebx, %eax

		/* Mapping 0x00000000 + %ecx * 4M => 0x00000000 + %ecx * 4M */
		movl %eax, (%esi, %ecx, 4)

		/* Mapping 0x80000000 + %ecx * 4M => 0x00000000 + %ecx * 4M */
		movl %eax, (%edi, %ecx, 4)
		addl $PAGE_SIZE, %ebx

		incl %ecx
		cmpl $512, %ecx

		jl floop

	movl %esi, %cr3

	movl %cr0, %ebx
	orl $CR0_PG, %ebx  /* paging on */
	movl %ebx, %cr0

	ret
FUNCTION_END(map_kernel_non_pse)

/** Print string to EGA display (in light green).
 *
 * Should be called from 32 bit protected mode with paging
 * turned off. A stack space of at least 24 bytes is required,
 * but the function does not establish a stack frame.
 *
 * Macros such as pm_status take care that this function
 * is used only when CONFIG_EGA is enabled.
 *
 * @param %esi NULL-terminated string to print.
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

/** Print string to EGA display.
 *
 * Should be called from 32 bit protected mode (with paging
 * enabled and stack established). This function is ABI compliant.
 *
 * If CONFIG_EGA is undefined or CONFIG_FB is defined
 * then this function does nothing.
 *
 * @param %ebp+0x08 NULL-terminated string to print.
 *
 */
early_puts:

#if ((defined(CONFIG_EGA)) && (!defined(CONFIG_FB)))

	/* Prologue, save preserved registers */
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	pushl %esi
	pushl %edi

	movl 0x08(%ebp), %esi
	movl $(PA2KA(0xb8000)), %edi  /* base of EGA text mode memory */
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
	jb early_puts_cursor_ok

		movw $1998, %ax

	early_puts_cursor_ok:

	movw %ax, %bx
	shl $1, %eax
	addl %eax, %edi

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
		movl %esi, %edx
		movl $(PA2KA(0xb80a0)), %esi
		movl $(PA2KA(0xb8000)), %edi
		movl $960, %ecx
		rep movsl

		/* Clear the 24th row */
		xorl %eax, %eax
		movl $40, %ecx
		rep stosl

		/* Go to row 24 */
		movl %edx, %esi
		movl $(PA2KA(0xb8f00)), %edi
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
	popl %edi
	popl %esi
	popl %ebx
	leave

#endif

	ret

.section K_BSS_START, "aw", @nobits

.align 4096
page_tables:
	.space 2*1024*1024;

.align 4096
page_directory:
	.space 4096, 0

SYMBOL(bootstrap_idtr)
	.word 0
	.long 0

SYMBOL(multiboot_eax)
	.long 0

SYMBOL(multiboot_ebx)
	.long 0

.section K_DATA_START, "aw", @progbits

SYMBOL(bootstrap_gdtr)
	.word GDT_SELECTOR(GDT_ITEMS)
	.long KA2PA(gdt)

status_prot:
	.asciz "[prot] "
status_pse:
	.asciz "[pse] "
status_non_pse:
	.asciz "[non_pse] "
status_multiboot_cmdline:
	.asciz "[multiboot_cmdline] "
status_prot2:
	.asciz "[prot2] "
status_prot3:
	.asciz "[prot3] "
status_main:
	.asciz "[main] "
