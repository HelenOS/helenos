#
# Copyright (C) 2005 Ondrej Palkovsky
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

#define IREGISTER_SPACE 120

#define IOFFSET_RAX 0x0
#define IOFFSET_RBX 0x8
#define IOFFSET_RCX 0x10
#define IOFFSET_RDX 0x18
#define IOFFSET_RSI 0x20
#define IOFFSET_RDI 0x28
#define IOFFSET_R8 0x30
#define IOFFSET_R9 0x38
#define IOFFSET_R10 0x40
#define IOFFSET_R11 0x48
#define IOFFSET_R12 0x50
#define IOFFSET_R13 0x58
#define IOFFSET_R14 0x60
#define IOFFSET_R15 0x68
#define IOFFSET_RBP 0x70

#  Mask for interrupts 0 - 31 (bits 0 - 31) where 0 means that int has no error word
# and 1 means interrupt with error word
#define ERROR_WORD_INTERRUPT_LIST 0x00027D00

#include <arch/pm.h>
#include <arch/mm/page.h>
	
.text
.global interrupt_handlers
.global syscall_entry
.global panic_printf

panic_printf:
	movq $halt, (%rsp)
	jmp printf

.global cpuid
.global has_cpuid
.global get_cycle
.global read_efer_flag
.global set_efer_flag
.global memcpy
.global memcpy_from_uspace
.global memcpy_to_uspace
.global memcpy_from_uspace_failover_address
.global memcpy_to_uspace_failover_address

#define MEMCPY_DST	%rdi
#define MEMCPY_SRC	%rsi
#define MEMCPY_SIZE	%rdx

/**
 * Copy memory from/to userspace.
 *
 * This is almost conventional memcpy().
 * The difference is that there is a failover part
 * to where control is returned from a page fault if
 * the page fault occurs during copy_from_uspace()
 * or copy_to_uspace().
 *
 * @param MEMCPY_DST	Destination address.
 * @param MEMCPY_SRC	Source address.
 * @param MEMCPY_SIZE	Number of bytes to copy.
 *
 * @retrun MEMCPY_SRC on success, 0 on failure.
 */
memcpy:
memcpy_from_uspace:
memcpy_to_uspace:
	movq MEMCPY_SRC, %rax

	movq MEMCPY_SIZE, %rcx
	shrq $3, %rcx			/* size / 8 */
	
	rep movsq			/* copy as much as possible word by word */

	movq MEMCPY_SIZE, %rcx
	andq $7, %rcx			/* size % 8 */
	jz 0f
	
	rep movsb			/* copy the rest byte by byte */
	
0:
	ret				/* return MEMCPY_SRC, success */

memcpy_from_uspace_failover_address:
memcpy_to_uspace_failover_address:
	xorq %rax, %rax			/* return 0, failure */
	ret

## Determine CPUID support
#
# Return 0 in EAX if CPUID is not support, 1 if supported.
#
has_cpuid:
	pushfq			# store flags
	popq %rax		# read flags
	movq %rax,%rdx		# copy flags
	btcl $21,%edx		# swap the ID bit
	pushq %rdx
	popfq			# propagate the change into flags
	pushfq
	popq %rdx		# read flags	
	andl $(1<<21),%eax	# interested only in ID bit
	andl $(1<<21),%edx
	xorl %edx,%eax		# 0 if not supported, 1 if supported
	ret

cpuid:
	movq %rbx, %r10  # we have to preserve rbx across function calls

	movl %edi,%eax	# load the command into %eax

	cpuid	
	movl %eax,0(%rsi)
	movl %ebx,4(%rsi)
	movl %ecx,8(%rsi)
	movl %edx,12(%rsi)

	movq %r10, %rbx
	ret

get_cycle:
	xorq %rax,%rax
	rdtsc
	ret

set_efer_flag:
	movq $0xc0000080, %rcx
	rdmsr
	btsl %edi, %eax
	wrmsr
	ret
	
read_efer_flag:	
	movq $0xc0000080, %rcx
	rdmsr
	ret 		

# Push all general purpose registers on stack except %rbp, %rsp
.macro save_all_gpr
	movq %rax, IOFFSET_RAX(%rsp)
	movq %rcx, IOFFSET_RCX(%rsp)
	movq %rdx, IOFFSET_RDX(%rsp)
	movq %rsi, IOFFSET_RSI(%rsp)
	movq %rdi, IOFFSET_RDI(%rsp)
	movq %r8, IOFFSET_R8(%rsp)
	movq %r9, IOFFSET_R9(%rsp)
	movq %r10, IOFFSET_R10(%rsp)
	movq %r11, IOFFSET_R11(%rsp)
#ifdef CONFIG_DEBUG_ALLREGS	
	movq %rbx, IOFFSET_RBX(%rsp)
	movq %rbp, IOFFSET_RBP(%rsp)
	movq %r12, IOFFSET_R12(%rsp)
	movq %r13, IOFFSET_R13(%rsp)
	movq %r14, IOFFSET_R14(%rsp)
	movq %r15, IOFFSET_R15(%rsp)
#endif
.endm

.macro restore_all_gpr
	movq IOFFSET_RAX(%rsp), %rax
	movq IOFFSET_RCX(%rsp), %rcx
	movq IOFFSET_RDX(%rsp), %rdx
	movq IOFFSET_RSI(%rsp), %rsi
	movq IOFFSET_RDI(%rsp), %rdi
	movq IOFFSET_R8(%rsp), %r8
	movq IOFFSET_R9(%rsp), %r9
	movq IOFFSET_R10(%rsp), %r10
	movq IOFFSET_R11(%rsp), %r11
#ifdef CONFIG_DEBUG_ALLREGS	
	movq IOFFSET_RBX(%rsp), %rbx
	movq IOFFSET_RBP(%rsp), %rbp
	movq IOFFSET_R12(%rsp), %r12
	movq IOFFSET_R13(%rsp), %r13
	movq IOFFSET_R14(%rsp), %r14
	movq IOFFSET_R15(%rsp), %r15
#endif
.endm

#ifdef CONFIG_DEBUG_ALLREGS
# define INTERRUPT_ALIGN 256
#else
# define INTERRUPT_ALIGN 128
#endif
	
## Declare interrupt handlers
#
# Declare interrupt handlers for n interrupt
# vectors starting at vector i.
#
# The handlers call exc_dispatch().
#
.macro handler i n

	/*
	 * Choose between version with error code and version without error code.
	 * Both versions have to be of the same size. amd64 assembly is, however,
	 * a little bit tricky. For instance, subq $0x80, %rsp and subq $0x78, %rsp
	 * can result in two instructions with different op-code lengths.
	 * Therefore we align the interrupt handlers.
	 */

	.iflt \i-32
		.if (1 << \i) & ERROR_WORD_INTERRUPT_LIST
			/*
			 * Version with error word.
			 */
			subq $IREGISTER_SPACE, %rsp
		.else
			/*
			 * Version without error word,
			 */
			subq $(IREGISTER_SPACE+8), %rsp
		.endif
	.else
		/*
		 * Version without error word,
		 */
		subq $(IREGISTER_SPACE+8), %rsp
	.endif	

	save_all_gpr

	movq $(\i), %rdi   	# %rdi - first parameter
	movq %rsp, %rsi   	# %rsi - pointer to istate
	call exc_dispatch 	# exc_dispatch(i, istate)
	
	restore_all_gpr
	# $8 = Skip error word
	addq $(IREGISTER_SPACE+8), %rsp
	iretq

	.align INTERRUPT_ALIGN
	.if (\n-\i)-1
	handler "(\i+1)",\n
	.endif
.endm

.align INTERRUPT_ALIGN
interrupt_handlers:
h_start:
	handler 0 IDT_ITEMS
h_end:

	
syscall_entry:
	# Switch to hidden gs	
	swapgs
	# %gs:0 now points to pointer to stack page
	mov %gs:0, %r10     # We have a ptr to stack page in r10
	addq $PAGE_SIZE-16, %r10 # We need some space to store old %sp
	
	movq %rsp, 0(%r10)  # Save old stack pointer to stack
	movq %r10, %rsp     # Change to new stack
	pushq %rcx          # Return address
	pushq %r11          # Save flags

	# Switch back to remain consistent
	swapgs 

	sti
	movq %r9, %rcx      # Exchange last parameter as a third
	
	call syscall_handler
	cli                 # We will be touching stack pointer
		
	popq %r11
	popq %rcx
	movq 0(%rsp), %rsp
	sysretq
		
		
.data
.global interrupt_handler_size

interrupt_handler_size: .quad (h_end-h_start)/IDT_ITEMS
