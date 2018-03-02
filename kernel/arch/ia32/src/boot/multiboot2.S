/*
 * Copyright (c) 2011 Martin Decky
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
#include <arch/pm.h>
#include <arch/cpuid.h>
#include <genarch/multiboot/multiboot2.h>

#define START_STACK  (BOOT_OFFSET - BOOT_STACK_SIZE)

.section K_TEXT_START, "ax"

.code32

.align 8
multiboot2_header_start:
	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT2_HEADER_ARCH_I386
	.long multiboot2_header_end - multiboot2_header_start
	.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCH_I386 + (multiboot2_header_end - multiboot2_header_start))

	/* Information request tag */
	.align 8
	tag_info_req_start:
		.word MULTIBOOT2_TAG_INFO_REQ
		.word MULTIBOOT2_FLAGS_REQUIRED
		.long tag_info_req_end - tag_info_req_start
		.long MULTIBOOT2_TAG_CMDLINE
		.long MULTIBOOT2_TAG_MODULE
		.long MULTIBOOT2_TAG_MEMMAP
#ifdef CONFIG_FB
		.long MULTIBOOT2_TAG_FBINFO
#endif
	tag_info_req_end:

	/* Address tag */
	.align 8
	tag_address_start:
		.word MULTIBOOT2_TAG_ADDRESS
		.word MULTIBOOT2_FLAGS_REQUIRED
		.long tag_address_end - tag_address_start
		.long multiboot2_header_start
		.long unmapped_ktext_start
		.long 0
		.long 0
	tag_address_end:

	/* Entry address tag */
	.align 8
	tag_entry_address_start:
		.word MULTIBOOT2_TAG_ENTRY_ADDRESS
		.word MULTIBOOT2_FLAGS_REQUIRED
		.long tag_entry_address_end - tag_entry_address_start
		.long multiboot2_image_start
	tag_entry_address_end:

	/* Flags tag */
	.align 8
	tag_flags_start:
		.word MULTIBOOT2_TAG_FLAGS
		.word MULTIBOOT2_FLAGS_REQUIRED
		.long tag_flags_end - tag_flags_start
		.long MULTIBOOT2_FLAGS_CONSOLE
	tag_flags_end:

#ifdef CONFIG_FB
	/* Framebuffer tag */
	.align 8
	tag_framebuffer_start:
		.word MULTIBOOT2_TAG_FRAMEBUFFER
		.word MULTIBOOT2_FLAGS_REQUIRED
		.long tag_framebuffer_end - tag_framebuffer_start
		.long CONFIG_BFB_WIDTH
		.long CONFIG_BFB_HEIGHT
		.long CONFIG_BFB_BPP
	tag_framebuffer_end:
#endif

	/* Module alignment tag */
	.align 8
	tag_module_align_start:
		.word MULTIBOOT2_TAG_MODULE_ALIGN
		.word MULTIBOOT2_FLAGS_REQUIRED
		.long tag_module_align_end - tag_module_align_start
		.long 0
	tag_module_align_end:

	/* Tag terminator */
	.align 8
	tag_terminator_start:
		.word MULTIBOOT2_TAG_TERMINATOR
		.word MULTIBOOT2_FLAGS_REQUIRED
		.long tag_terminator_end - tag_terminator_start
	tag_terminator_end:
multiboot2_header_end:

SYMBOL(multiboot2_image_start)
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

	jmpl $GDT_SELECTOR(KTEXT_DES), $multiboot2_meeting_point
	multiboot2_meeting_point:

	/* Save multiboot arguments */
	movl %eax, multiboot_eax
	movl %ebx, multiboot_ebx

#ifndef PROCESSOR_i486

	movl $(INTEL_CPUID_LEVEL), %eax
	cpuid
	cmp $0x0, %eax  /* any function > 0? */
	jbe pse_unsupported

	movl $(INTEL_CPUID_STANDARD), %eax
	cpuid
	bt $(INTEL_PSE), %edx
	jnc pse_unsupported

		/* Map kernel and turn paging on */
		call map_kernel_pse
		jmp stack_init

#endif /* PROCESSOR_i486 */

	pse_unsupported:

		/* Map kernel and turn paging on */
		call map_kernel_non_pse

	stack_init:

	/* Create the first stack frame */
	pushl $0
	movl %esp, %ebp

	/* Call ia32_pre_main(multiboot_eax, multiboot_ebx) */
	pushl multiboot_ebx
	pushl multiboot_eax
	call ia32_pre_main

	/* Call main_bsp() */
	call main_bsp

	/* Not reached */
	cli
	hlt0:
		hlt
		jmp hlt0
