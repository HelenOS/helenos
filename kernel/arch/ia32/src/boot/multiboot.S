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
	pushl \msg
	call early_puts
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

#include "vesa_prot.inc"

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

	call calc_kernel_end
	call find_mem_for_pt

	mov kernel_end, %esi
	mov free_area, %ecx

	cmpl %esi, %ecx
	jbe use_kernel_end

		mov %ecx, %esi

		/* Align address down to 4k */
		andl $(~(PAGE_SIZE - 1)), %esi

	use_kernel_end:

		/* Align address to 4k */
		addl $(PAGE_SIZE - 1), %esi
		andl $(~(PAGE_SIZE - 1)), %esi

		/* Allocate space for page tables */
		movl %esi, pt_loc
		movl $KA2PA(ballocs), %edi

		movl %esi, (%edi)
		addl $4, %edi
		movl $(2 * 1024 * 1024), (%edi)

		/* Fill page tables */
		xorl %ecx, %ecx
		xorl %ebx, %ebx

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
		movl pt_loc, %ebx

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

/** Calculate unmapped address of the end of the kernel. */
calc_kernel_end:
	movl $KA2PA(hardcoded_load_address), %edi
	movl (%edi), %esi
	leal KA2PA(0)(%esi), %esi

	movl $KA2PA(hardcoded_ktext_size), %edi
	addl (%edi), %esi
	leal KA2PA(0)(%esi), %esi

	movl $KA2PA(hardcoded_kdata_size), %edi
	addl (%edi), %esi
	leal KA2PA(0)(%esi), %esi
	movl %esi, kernel_end

	ret

/** Find free 2M (+4k for alignment) region where to store page tables */
find_mem_for_pt:
	/* Check if multiboot info is present */
	cmpl $MULTIBOOT_LOADER_MAGIC, multiboot_eax
	je check_multiboot_map

		ret

	check_multiboot_map:

		/* Copy address of the multiboot info to ebx */
		movl multiboot_ebx, %ebx

		/* Check if memory map flag is present */
		movl (%ebx), %edx
		andl $MULTIBOOT_INFO_FLAGS_MMAP, %edx
		jnz use_multiboot_map

			ret

	use_multiboot_map:

		/* Copy address of the memory map to edx */
		movl MULTIBOOT_INFO_OFFSET_MMAP_ADDR(%ebx), %edx
		movl %edx, %ecx

		addl MULTIBOOT_INFO_OFFSET_MMAP_LENGTH(%ebx), %ecx

		/* Find a free region at least 2M in size */
		check_memmap_loop:

			/* Is this a free region? */
			cmpl $MEMMAP_MEMORY_AVAILABLE, MULTIBOOT_MEMMAP_OFFSET_MM_INFO + E820MEMMAP_OFFSET_TYPE(%edx)
			jnz next_region

			/* Check size */
			cmpl $0, MULTIBOOT_MEMMAP_OFFSET_MM_INFO + E820MEMMAP_OFFSET_SIZE + 4(%edx)
			jnz next_region
			cmpl $(2 * 1024 * 1024 + PAGE_SIZE), MULTIBOOT_MEMMAP_OFFSET_MM_INFO + E820MEMMAP_OFFSET_SIZE(%edx)
			jbe next_region

			cmpl $0, MULTIBOOT_MEMMAP_OFFSET_MM_INFO + E820MEMMAP_OFFSET_BASE_ADDRESS + 4(%edx)
			jz found_region

		next_region:

			cmp %ecx, %edx
			jbe next_region_do

				ret

		next_region_do:

			addl MULTIBOOT_MEMMAP_OFFSET_SIZE(%edx), %edx
			addl $MULTIBOOT_MEMMAP_SIZE_SIZE, %edx
			jmp check_memmap_loop

		found_region:

			/* Use end of the found region */
			mov MULTIBOOT_MEMMAP_OFFSET_MM_INFO + E820MEMMAP_OFFSET_BASE_ADDRESS(%edx), %ecx
			add MULTIBOOT_MEMMAP_OFFSET_MM_INFO + E820MEMMAP_OFFSET_SIZE(%edx), %ecx
			sub $(2 * 1024 * 1024), %ecx
			mov %ecx, free_area

			ret

/** Print string to EGA display (in light red) and halt.
 *
 * Should be executed from 32 bit protected mode with paging
 * turned off. Stack is not required. This routine is used even
 * if CONFIG_EGA is not enabled. Since we are going to halt the
 * CPU anyway, it is always better to at least try to print
 * some hints.
 *
 * @param %esi NULL-terminated string to print.
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

#include "vesa_real.inc"

.section K_DATA_START, "aw", @progbits

.align 4096
page_directory:
	.space 4096, 0

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

pt_loc:
	.long 0
kernel_end:
	.long 0
free_area:
	.long 0

status_prot:
	.asciz "[prot] "
status_pse:
	.asciz "[pse] "
status_non_pse:
	.asciz "[non_pse] "
status_vesa_copy:
	.asciz "[vesa_copy] "
status_multiboot_cmdline:
	.asciz "[multiboot_cmdline] "
status_vesa_real:
	.asciz "[vesa_real] "
status_prot2:
	.asciz "[prot2] "
status_prot3:
	.asciz "[prot3] "
status_main:
	.asciz "[main] "
