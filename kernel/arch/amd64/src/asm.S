/*
 * Copyright (c) 2005 Ondrej Palkovsky
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
#include <arch/pm.h>
#include <arch/mm/page.h>
#include <arch/istate_struct.h>
#include <arch/kseg_struct.h>
#include <arch/cpu.h>
#include <arch/smp/apic.h>

.text

#define MEMCPY_DST   %rdi
#define MEMCPY_SRC   %rsi
#define MEMCPY_SIZE  %rdx

/**
 * Copy memory from/to userspace.
 *
 * This is almost conventional memcpy().
 * The difference is that there is a failover part
 * to where control is returned from a page fault if
 * the page fault occurs during copy_from_uspace()
 * or copy_to_uspace().
 *
 * @param MEMCPY_DST  Destination address.
 * @param MEMCPY_SRC  Source address.
 * @param MEMCPY_SIZE Number of bytes to copy.
 *
 * @retrun MEMCPY_DST on success, 0 on failure.
 *
 */
FUNCTION_BEGIN(memcpy_from_uspace)
FUNCTION_BEGIN(memcpy_to_uspace)
	movq MEMCPY_DST, %rax

	movq MEMCPY_SIZE, %rcx
	shrq $3, %rcx           /* size / 8 */

	rep movsq               /* copy as much as possible word by word */

	movq MEMCPY_SIZE, %rcx
	andq $7, %rcx           /* size % 8 */
	jz 0f

	rep movsb               /* copy the rest byte by byte */

	0:
		ret                 /* return MEMCPY_SRC, success */
FUNCTION_END(memcpy_from_uspace)
FUNCTION_END(memcpy_to_uspace)

SYMBOL(memcpy_from_uspace_failover_address)
SYMBOL(memcpy_to_uspace_failover_address)
	xorl %eax, %eax         /* return 0, failure */
	ret

/** Determine CPUID support
*
* @return 0 in EAX if CPUID is not support, 1 if supported.
*
*/
FUNCTION_BEGIN(has_cpuid)
	/* Load RFLAGS */
	pushfq
	popq %rax
	movq %rax, %rdx

	/* Flip the ID bit */
	xorl $RFLAGS_ID, %edx

	/* Store RFLAGS */
	pushq %rdx
	popfq
	pushfq

	/* Get the ID bit again */
	popq %rdx
	andl $RFLAGS_ID, %eax
	andl $RFLAGS_ID, %edx

	/* 0 if not supported, 1 if supported */
	xorl %edx, %eax
	ret
FUNCTION_END(has_cpuid)

FUNCTION_BEGIN(cpuid)
	/* Preserve %rbx across function calls */
	movq %rbx, %r10

	/* Load the command into %eax */
	movl %edi, %eax

	cpuid
	movl %eax, 0(%rsi)
	movl %ebx, 4(%rsi)
	movl %ecx, 8(%rsi)
	movl %edx, 12(%rsi)

	movq %r10, %rbx
	ret
FUNCTION_END(cpuid)

/** Enable local APIC
 *
 * Enable local APIC in MSR.
 *
 */
FUNCTION_BEGIN(enable_l_apic_in_msr)
	movl $AMD_MSR_APIC_BASE, %ecx
	rdmsr
	orl $(L_APIC_BASE | AMD_APIC_BASE_GE), %eax
	wrmsr
	ret
FUNCTION_END(enable_l_apic_in_msr)

/*
 * Size of the istate structure without the hardware-saved part and without the
 * error word.
 */
#define ISTATE_SOFT_SIZE	ISTATE_SIZE - (6 * 8)

/**
 * Mask for interrupts 0 - 31 (bits 0 - 31) where 0 means that int
 * has no error word  and 1 means interrupt with error word
 *
 */
#define ERROR_WORD_INTERRUPT_LIST  0x00027D00

.macro handler i
SYMBOL(int_\i)

	/*
	 * Choose between version with error code and version without error
	 * code.
	 */

	.iflt \i-32
		.if (1 << \i) & ERROR_WORD_INTERRUPT_LIST
			/*
			 * Version with error word.
			 */
			subq $ISTATE_SOFT_SIZE, %rsp
		.else
			/*
			 * Version without error word.
			 */
			subq $(ISTATE_SOFT_SIZE + 8), %rsp
		.endif
	.else
		/*
		 * Version without error word.
		 */
		subq $(ISTATE_SOFT_SIZE + 8), %rsp
	.endif

	/*
	 * Save the general purpose registers.
	 */
	movq %rax, ISTATE_OFFSET_RAX(%rsp)
	movq %rbx, ISTATE_OFFSET_RBX(%rsp)
	movq %rcx, ISTATE_OFFSET_RCX(%rsp)
	movq %rdx, ISTATE_OFFSET_RDX(%rsp)
	movq %rsi, ISTATE_OFFSET_RSI(%rsp)
	movq %rdi, ISTATE_OFFSET_RDI(%rsp)
	movq %rbp, ISTATE_OFFSET_RBP(%rsp)
	movq %r8, ISTATE_OFFSET_R8(%rsp)
	movq %r9, ISTATE_OFFSET_R9(%rsp)
	movq %r10, ISTATE_OFFSET_R10(%rsp)
	movq %r11, ISTATE_OFFSET_R11(%rsp)
	movq %r12, ISTATE_OFFSET_R12(%rsp)
	movq %r13, ISTATE_OFFSET_R13(%rsp)
	movq %r14, ISTATE_OFFSET_R14(%rsp)
	movq %r15, ISTATE_OFFSET_R15(%rsp)

	/*
	 * Is this trap from the kernel?
	 */
	cmpq $(GDT_SELECTOR(KTEXT_DES)), ISTATE_OFFSET_CS(%rsp)
	jz 0f

	/*
	 * Switch to kernel FS base.
	 */
	swapgs
	movl $AMD_MSR_FS, %ecx
	movl %gs:KSEG_OFFSET_FSBASE, %eax
	movl %gs:KSEG_OFFSET_FSBASE+4, %edx
	wrmsr
	swapgs

	/*
	 * Imitate a regular stack frame linkage.
	 * Stop stack traces here if we came from userspace.
	 */
0:	movl $0x0, %edx
	cmovnzq %rdx, %rbp

	movq %rbp, ISTATE_OFFSET_RBP_FRAME(%rsp)
	movq ISTATE_OFFSET_RIP(%rsp), %rax
	movq %rax, ISTATE_OFFSET_RIP_FRAME(%rsp)
	leaq ISTATE_OFFSET_RBP_FRAME(%rsp), %rbp

	movq $(\i), %rdi   /* pass intnum in the first argument */
	movq %rsp, %rsi    /* pass istate address in the second argument */

	cld

	/* Call exc_dispatch(i, istate) */
	call exc_dispatch

	/*
	 * Restore all scratch registers and the preserved registers we have
	 * clobbered in this handler (i.e. RBP).
	 */
	movq ISTATE_OFFSET_RAX(%rsp), %rax
	movq ISTATE_OFFSET_RCX(%rsp), %rcx
	movq ISTATE_OFFSET_RDX(%rsp), %rdx
	movq ISTATE_OFFSET_RSI(%rsp), %rsi
	movq ISTATE_OFFSET_RDI(%rsp), %rdi
	movq ISTATE_OFFSET_RBP(%rsp), %rbp
	movq ISTATE_OFFSET_R8(%rsp), %r8
	movq ISTATE_OFFSET_R9(%rsp), %r9
	movq ISTATE_OFFSET_R10(%rsp), %r10
	movq ISTATE_OFFSET_R11(%rsp), %r11

	/* $8 = Skip error word */
	addq $(ISTATE_SOFT_SIZE + 8), %rsp
	iretq
.endm

#define LIST_0_63 \
	0, 1, 2, 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,\
	28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,\
	53,54,55,56,57,58,59,60,61,62,63

SYMBOL(interrupt_handlers)
.irp cnt, LIST_0_63
	handler \cnt
.endr

/** Low-level syscall handler
 *
 * Registers on entry:
 *
 * @param %rcx Userspace return address.
 * @param %r11 Userspace RLFAGS.
 *
 * @param %rax Syscall number.
 * @param %rdi 1st syscall argument.
 * @param %rsi 2nd syscall argument.
 * @param %rdx 3rd syscall argument.
 * @param %r10 4th syscall argument. Used instead of RCX because
 *             the SYSCALL instruction clobbers it.
 * @param %r8  5th syscall argument.
 * @param %r9  6th syscall argument.
 *
 * @return Return value is in %rax.
 *
 */
SYMBOL(syscall_entry)
	/* Switch to hidden %gs */
	swapgs

	movq %rsp, %gs:KSEG_OFFSET_USTACK_RSP  /* save this thread's user RSP */
	movq %gs:KSEG_OFFSET_KSTACK_RSP, %rsp  /* set this thread's kernel RSP */

	/*
	 * Note that the space needed for the imitated istate structure has been
	 * preallocated for us in thread_create_arch() and set in
	 * before_thread_runs_arch().
	 */

	/*
	 * Save the general purpose registers and push the 7th argument (syscall
	 * number) onto the stack. Note that the istate structure has a layout
	 * which supports this.
	 */
	movq %rax, ISTATE_OFFSET_RAX(%rsp)  /* 7th argument, passed on stack */
	movq %rbx, ISTATE_OFFSET_RBX(%rsp)  /* observability */
	movq %rcx, ISTATE_OFFSET_RCX(%rsp)  /* userspace RIP */
	movq %rdx, ISTATE_OFFSET_RDX(%rsp)  /* 3rd argument, observability */
	movq %rsi, ISTATE_OFFSET_RSI(%rsp)  /* 2nd argument, observability */
	movq %rdi, ISTATE_OFFSET_RDI(%rsp)  /* 1st argument, observability */
	movq %rbp, ISTATE_OFFSET_RBP(%rsp)  /* need to preserve userspace RBP */
	movq %r8, ISTATE_OFFSET_R8(%rsp)    /* 5th argument, observability */
	movq %r9, ISTATE_OFFSET_R9(%rsp)    /* 6th argument, observability */
	movq %r10, ISTATE_OFFSET_R10(%rsp)  /* 4th argument, observability */
	movq %r11, ISTATE_OFFSET_R11(%rsp)  /* low 32 bits userspace RFLAGS */
	movq %r12, ISTATE_OFFSET_R12(%rsp)  /* observability */
	movq %r13, ISTATE_OFFSET_R13(%rsp)  /* observability */
	movq %r14, ISTATE_OFFSET_R14(%rsp)  /* observability */
	movq %r15, ISTATE_OFFSET_R15(%rsp)  /* observability */

	/*
	 * Switch to kernel FS base.
	 */
	movl $AMD_MSR_FS, %ecx
	movl %gs:KSEG_OFFSET_FSBASE, %eax
	movl %gs:KSEG_OFFSET_FSBASE+4, %edx
	wrmsr
	movq ISTATE_OFFSET_RDX(%rsp), %rdx	/* restore 3rd argument */

	/*
	 * Save the return address and the userspace stack on locations that
	 * would normally be taken by them.
	 */
	movq %gs:KSEG_OFFSET_USTACK_RSP, %rax
	movq %rax, ISTATE_OFFSET_RSP(%rsp)
	movq %rcx, ISTATE_OFFSET_RIP(%rsp)

	/*
	 * Imitate a regular stack frame linkage.
	 */
	movq $0, ISTATE_OFFSET_RBP_FRAME(%rsp)
	movq %rcx, ISTATE_OFFSET_RIP_FRAME(%rsp)
	leaq ISTATE_OFFSET_RBP_FRAME(%rsp), %rbp

	/* Switch back to normal %gs */
	swapgs
	sti

	/* Copy the 4th argument where it is expected  */
	movq %r10, %rcx

	/*
	 * Call syscall_handler() with the 7th argument passed on stack.
	 */
	call syscall_handler

	/*
	 * Test if the saved return address is canonical and not-kernel.
	 * We do this by looking at the 16 most significant bits
	 * of the saved return address (two bytes at offset 6).
	 */
	testw $0xffff, ISTATE_OFFSET_RIP+6(%rsp)
	jnz bad_rip

	cli

	/*
	 * Restore registers needed for return via the SYSRET instruction and
	 * the clobbered preserved registers (i.e. RBP).
	 */
	movq ISTATE_OFFSET_RBP(%rsp), %rbp
	movq ISTATE_OFFSET_RCX(%rsp), %rcx
	movq ISTATE_OFFSET_R11(%rsp), %r11
	movq ISTATE_OFFSET_RSP(%rsp), %rsp

	/*
	 * Clear the rest of the scratch registers to prevent information leak.
	 * The 32-bit XOR on the low GPRs actually clears the entire 64-bit
	 * register and the instruction is shorter.
	 */
	xorl %edx, %edx
	xorl %esi, %esi
	xorl %edi, %edi
	xorq %r8, %r8
	xorq %r9, %r9
	xorq %r10, %r10

	sysretq

bad_rip:
	movq %rsp, %rdi
	movabs $bad_rip_msg, %rsi
	xorb %al, %al
	callq fault_from_uspace
	/* not reached */

bad_rip_msg:
	.asciz "Invalid instruction pointer."

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
 * @param %rdi Unicode character to be printed.
 *
 */
FUNCTION_BEGIN(early_putchar)
#if ((defined(CONFIG_EGA)) && (!defined(CONFIG_FB)))

	/* Prologue, save preserved registers */
	pushq %rbp
	movq %rsp, %rbp
	pushq %rbx

	movq %rdi, %rsi
	movq $(PA2KA(0xb8000)), %rdi  /* base of EGA text mode memory */
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
	shl $1, %rax
	addq %rax, %rdi

	movq %rsi, %rax

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
		movq $(PA2KA(0xb80a0)), %rsi
		movq $(PA2KA(0xb8000)), %rdi
		movl $480, %ecx
		rep movsq

		/* Clear the 24th row */
		xorl %eax, %eax
		movl $20, %ecx
		rep stosq

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
	popq %rbx
	leave

#endif

	ret
FUNCTION_END(early_putchar)
