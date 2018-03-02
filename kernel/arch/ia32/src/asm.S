/*
 * Copyright (c) 2010 Jakub Jermar
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

/** Very low and hardware-level functions
 *
 */

#include <abi/asmtool.h>
#include <arch/pm.h>
#include <arch/cpu.h>
#include <arch/mm/page.h>
#include <arch/istate_struct.h>
#include <arch/smp/apic.h>

.text

#define MEMCPY_DST   4
#define MEMCPY_SRC   8
#define MEMCPY_SIZE  12

/** Copy memory to/from userspace.
 *
 * This is almost conventional memcpy().
 * The difference is that there is a failover part
 * to where control is returned from a page fault
 * if the page fault occurs during copy_from_uspace()
 * or copy_to_uspace().
 *
 * @param MEMCPY_DST(%esp)  Destination address.
 * @param MEMCPY_SRC(%esp)  Source address.
 * @param MEMCPY_SIZE(%esp) Size.
 *
 * @return MEMCPY_DST(%esp) on success and 0 on failure.
 *
 */
FUNCTION_BEGIN(memcpy_from_uspace)
FUNCTION_BEGIN(memcpy_to_uspace)
	movl %edi, %edx  /* save %edi */
	movl %esi, %eax  /* save %esi */

	movl MEMCPY_SIZE(%esp), %ecx
	shrl $2, %ecx  /* size / 4 */

	movl MEMCPY_DST(%esp), %edi
	movl MEMCPY_SRC(%esp), %esi

	/* Copy whole words */
	rep movsl

	movl MEMCPY_SIZE(%esp), %ecx
	andl $3, %ecx  /* size % 4 */
	jz 0f

	/* Copy the rest byte by byte */
	rep movsb

	0:

		movl %edx, %edi
		movl %eax, %esi

		/* MEMCPY_DST(%esp), success */
		movl MEMCPY_DST(%esp), %eax
		ret
FUNCTION_END(memcpy_from_uspace)
FUNCTION_END(memcpy_to_uspace)

/*
 * We got here from as_page_fault() after the memory operations
 * above had caused a page fault.
 */
SYMBOL(memcpy_from_uspace_failover_address)
SYMBOL(memcpy_to_uspace_failover_address)
	movl %edx, %edi
	movl %eax, %esi

	/* Return 0, failure */
	xorl %eax, %eax
	ret

/** Turn paging on
 *
 * Enable paging and write-back caching in CR0.
 *
 */
FUNCTION_BEGIN(paging_on)
	movl %cr0, %edx
	orl $CR0_PG, %edx  /* paging on */

	/* Clear Cache Disable and not Write Though */
	andl $~(CR0_CD | CR0_NW), %edx
	movl %edx, %cr0
	jmp 0f

	0:
		ret
FUNCTION_END(paging_on)

/** Enable local APIC
 *
 * Enable local APIC in MSR.
 *
 */
FUNCTION_BEGIN(enable_l_apic_in_msr)
	movl $IA32_MSR_APIC_BASE, %ecx
	rdmsr
	orl $(L_APIC_BASE | IA32_APIC_BASE_GE), %eax
	wrmsr
	ret
FUNCTION_END(enable_l_apic_in_msr)

/*
 * Size of the istate structure without the hardware-saved part
 * and without the error word.
 */
#define ISTATE_SOFT_SIZE	ISTATE_SIZE - (6 * 4)

/*
 * The SYSENTER syscall mechanism can be used for syscalls with
 * four or fewer arguments. To pass these four arguments, we
 * use four registers: EDX, ECX, EBX, ESI. The syscall number
 * is passed in EAX. We use EDI to remember the return address
 * and EBP to remember the stack. The INT-based syscall mechanism
 * can actually handle six arguments plus the syscall number
 * entirely in registers.
 */
SYMBOL(sysenter_handler)

	/*
	 * Note that the space needed for the istate structure has been
	 * preallocated on the stack by before_thread_runs_arch().
	 */

	/*
	 * Save the return address and the userspace stack in the istate
	 * structure on locations that would normally be taken by them.
	 */
	movl %ebp, ISTATE_OFFSET_ESP(%esp)
	movl %edi, ISTATE_OFFSET_EIP(%esp)

	/*
	 * Push syscall arguments onto the stack
	 */
	movl %eax, ISTATE_OFFSET_EAX(%esp)
	movl %ebx, ISTATE_OFFSET_EBX(%esp)
	movl %ecx, ISTATE_OFFSET_ECX(%esp)
	movl %edx, ISTATE_OFFSET_EDX(%esp)
	movl %esi, ISTATE_OFFSET_ESI(%esp)
	movl %edi, ISTATE_OFFSET_EDI(%esp)	/* observability; not needed */
	movl %ebp, ISTATE_OFFSET_EBP(%esp)	/* observability; not needed */

	/*
	 * Fake up the stack trace linkage.
	 */
	movl %edi, ISTATE_OFFSET_EIP_FRAME(%esp)
	movl $0, ISTATE_OFFSET_EBP_FRAME(%esp)
	leal ISTATE_OFFSET_EBP_FRAME(%esp), %ebp

	/*
	 * Switch to kernel selectors.
	 */
	movl $(GDT_SELECTOR(KDATA_DES)), %eax
	movl %eax, %ds
	movl %eax, %es
	movl $(GDT_SELECTOR(VREG_DES)), %eax
	movl %eax, %gs

	/*
	 * Sanitize EFLAGS.
	 *
	 * SYSENTER does not clear the NT flag, which could thus proliferate
	 * from here to the IRET instruction via a context switch and result
	 * in crash.
	 *
	 * SYSENTER does not clear DF, which the ABI assumes to be cleared.
	 *
	 * SYSENTER clears IF, which we would like to be set for syscalls.
	 *
	 */
	pushl $(EFLAGS_IF)  /* specify EFLAGS bits that we want to set */
	popfl	            /* set bits from the mask, clear or ignore others */

	call syscall_handler

	/*
	 * Prepare return address and userspace stack for SYSEXIT.
	 */
	movl ISTATE_OFFSET_EIP(%esp), %edx
	movl ISTATE_OFFSET_ESP(%esp), %ecx

	sysexit   /* return to userspace */

/*
 * This is the legacy syscall handler using the interrupt mechanism.
 */
SYMBOL(int_syscall)
	subl $(ISTATE_SOFT_SIZE + 4), %esp

	/*
	 * Push syscall arguments onto the stack
	 *
	 * NOTE: The idea behind the order of arguments passed
	 *       in registers is to use all scratch registers
	 *       first and preserved registers next. An optimized
	 *       libc syscall wrapper can make use of this setup.
	 *       The istate structure is arranged in the way to support
	 *       this idea.
	 *
	 */
	movl %eax, ISTATE_OFFSET_EAX(%esp)
	movl %ebx, ISTATE_OFFSET_EBX(%esp)
	movl %ecx, ISTATE_OFFSET_ECX(%esp)
	movl %edx, ISTATE_OFFSET_EDX(%esp)
	movl %edi, ISTATE_OFFSET_EDI(%esp)
	movl %esi, ISTATE_OFFSET_ESI(%esp)
	movl %ebp, ISTATE_OFFSET_EBP(%esp)

	/*
	 * Save the segment registers.
	 */
	movl %gs, %ecx
	movl %fs, %edx

	movl %ecx, ISTATE_OFFSET_GS(%esp)
	movl %edx, ISTATE_OFFSET_FS(%esp)

	movl %es, %ecx
	movl %ds, %edx

	movl %ecx, ISTATE_OFFSET_ES(%esp)
	movl %edx, ISTATE_OFFSET_DS(%esp)

	/*
	 * Switch to kernel selectors.
	 */
	movl $(GDT_SELECTOR(KDATA_DES)), %eax
	movl %eax, %ds
	movl %eax, %es
	movl $(GDT_SELECTOR(VREG_DES)), %eax
	movl %eax, %gs

	movl $0, ISTATE_OFFSET_EBP_FRAME(%esp)
	movl ISTATE_OFFSET_EIP(%esp), %eax
	movl %eax, ISTATE_OFFSET_EIP_FRAME(%esp)
	leal ISTATE_OFFSET_EBP_FRAME(%esp), %ebp

	cld

	/* Call syscall_handler(edx, ecx, ebx, esi, edi, ebp, eax) */
	call syscall_handler

	/*
	 * Restore the segment registers.
	 */
	movl ISTATE_OFFSET_GS(%esp), %ecx
	movl ISTATE_OFFSET_FS(%esp), %edx

	movl %ecx, %gs
	movl %edx, %fs

	movl ISTATE_OFFSET_ES(%esp), %ecx
	movl ISTATE_OFFSET_DS(%esp), %edx

	movl %ecx, %es
	movl %edx, %ds

	/*
	 * Restore the preserved registers the handler cloberred itself
	 * (i.e. EBP).
	 */
	movl ISTATE_OFFSET_EBP(%esp), %ebp

	addl $(ISTATE_SOFT_SIZE + 4), %esp
	iret

/**
 * Mask for interrupts 0 - 31 (bits 0 - 31) where 0 means that int
 * has no error word  and 1 means interrupt with error word
 *
 */
#define ERROR_WORD_INTERRUPT_LIST  0x00027d00

.macro handler i
SYMBOL(int_\i)
	/*
	 * This macro distinguishes between two versions of ia32
	 * exceptions. One version has error word and the other
	 * does not have it. The latter version fakes the error
	 * word on the stack so that the handlers and istate_t
	 * can be the same for both types.
	 */
	.iflt \i - 32
		.if (1 << \i) & ERROR_WORD_INTERRUPT_LIST
			/*
			 * Exception with error word.
			 */
			subl $ISTATE_SOFT_SIZE, %esp
		.else
			/*
			 * Exception without error word: fake up one
			 */
			subl $(ISTATE_SOFT_SIZE + 4), %esp
		.endif
	.else
		/*
		 * Interrupt: fake up an error word
		 */
		subl $(ISTATE_SOFT_SIZE + 4), %esp
	.endif

	/*
	 * Save the general purpose registers.
	 */
	movl %eax, ISTATE_OFFSET_EAX(%esp)
	movl %ebx, ISTATE_OFFSET_EBX(%esp)
	movl %ecx, ISTATE_OFFSET_ECX(%esp)
	movl %edx, ISTATE_OFFSET_EDX(%esp)
	movl %edi, ISTATE_OFFSET_EDI(%esp)
	movl %esi, ISTATE_OFFSET_ESI(%esp)
	movl %ebp, ISTATE_OFFSET_EBP(%esp)

	/*
	 * Save the segment registers.
	 */
	movl %gs, %ecx
	movl %fs, %edx

	movl %ecx, ISTATE_OFFSET_GS(%esp)
	movl %edx, ISTATE_OFFSET_FS(%esp)

	movl %es, %ecx
	movl %ds, %edx

	movl %ecx, ISTATE_OFFSET_ES(%esp)
	movl %edx, ISTATE_OFFSET_DS(%esp)

	/*
	 * Switch to kernel selectors.
	 */
	movl $(GDT_SELECTOR(KDATA_DES)), %eax
	movl %eax, %ds
	movl %eax, %es
	movl $(GDT_SELECTOR(VREG_DES)), %eax
	movl %eax, %gs

	/*
	 * Imitate a regular stack frame linkage.
	 * Stop stack traces here if we came from userspace.
	 */
	xorl %eax, %eax
	cmpl $(GDT_SELECTOR(KTEXT_DES)), ISTATE_OFFSET_CS(%esp)
#ifdef PROCESSOR_i486
	jz 0f
		movl %eax, %ebp
	0:
#else
	cmovnzl %eax, %ebp
#endif

	movl %ebp, ISTATE_OFFSET_EBP_FRAME(%esp)
	movl ISTATE_OFFSET_EIP(%esp), %eax
	movl %eax, ISTATE_OFFSET_EIP_FRAME(%esp)
	leal ISTATE_OFFSET_EBP_FRAME(%esp), %ebp

	cld

	pushl %esp   /* pass istate address */
	pushl $(\i)  /* pass intnum */

	/* Call exc_dispatch(intnum, istate) */
	call exc_dispatch

	addl $8, %esp  /* clear arguments from the stack */

	/*
	 * Restore the selector registers.
	 */
	movl ISTATE_OFFSET_GS(%esp), %ecx
	movl ISTATE_OFFSET_FS(%esp), %edx

	movl %ecx, %gs
	movl %edx, %fs

	movl ISTATE_OFFSET_ES(%esp), %ecx
	movl ISTATE_OFFSET_DS(%esp), %edx

	movl %ecx, %es
	movl %edx, %ds

	/*
	 * Restore the scratch registers and the preserved
	 * registers the handler cloberred itself
	 * (i.e. EBP).
	 */
	movl ISTATE_OFFSET_EAX(%esp), %eax
	movl ISTATE_OFFSET_ECX(%esp), %ecx
	movl ISTATE_OFFSET_EDX(%esp), %edx
	movl ISTATE_OFFSET_EBP(%esp), %ebp

	addl $(ISTATE_SOFT_SIZE + 4), %esp
	iret
.endm

#define LIST_0_63 \
	0, 1, 2, 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,\
	28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,\
	53,54,55,56,57,58,59,60,61,62,63

interrupt_handlers:
.irp cnt, LIST_0_63
	handler \cnt
.endr

/** Print Unicode character to EGA display.
 *
 * If CONFIG_EGA is undefined or CONFIG_FB is defined
 * then this function does nothing.
 *
 * Since the EGA can only display Extended ASCII (usually
 * ISO Latin 1) characters, some of the Unicode characters
 * can be displayed in a wrong way. Only newline and backspace
 * are interpreted, all other characters (even unprintable) are
 * printed verbatim.
 *
 * @param %ebp+0x08 Unicode character to be printed.
 *
 */
FUNCTION_BEGIN(early_putchar)

#if ((defined(CONFIG_EGA)) && (!defined(CONFIG_FB)))

	/* Prologue, save preserved registers */
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	pushl %esi
	pushl %edi

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
	jb early_putchar_cursor_ok

		movw $1998, %ax

	early_putchar_cursor_ok:

	movw %ax, %bx
	shl $1, %eax
	addl %eax, %edi

	movl 0x08(%ebp), %eax

	cmp $0x0a, %al
	jne early_putchar_backspace

		/* Interpret newline */

		movw %bx, %ax  /* %bx -> %dx:%ax */
		xorw %dx, %dx

		movw $80, %cx
		idivw %cx, %ax  /* %dx = %bx % 80 */

		/* %bx <- %bx + 80 - (%bx % 80) */
		addw %cx, %bx
		subw %dx, %bx

		jmp early_putchar_skip

	early_putchar_backspace:

		cmp $0x08, %al
		jne early_putchar_print

		/* Interpret backspace */

		cmp $0x0000, %bx
		je early_putchar_skip

		dec %bx
		jmp early_putchar_skip

	early_putchar_print:

		/* Print character */

		movb $0x0e, %ah  /* black background, yellow foreground */
		stosw
		inc %bx

	early_putchar_skip:

	/* Sanity check for the cursor on the last line */
	cmp $2000, %bx
	jb early_putchar_no_scroll

		/* Scroll the screen (24 rows) */
		movl $(PA2KA(0xb80a0)), %esi
		movl $(PA2KA(0xb8000)), %edi
		movl $960, %ecx
		rep movsl

		/* Clear the 24th row */
		xorl %eax, %eax
		movl $40, %ecx
		rep stosl

		/* Go to row 24 */
		movw $1920, %bx

	early_putchar_no_scroll:

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
FUNCTION_END(early_putchar)

