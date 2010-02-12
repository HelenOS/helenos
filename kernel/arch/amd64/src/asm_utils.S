#
# Copyright (c) 2005 Ondrej Palkovsky
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

#define IREGISTER_SPACE	80

#define IOFFSET_RAX	0x0
#define IOFFSET_RCX	0x8
#define IOFFSET_RDX	0x10
#define IOFFSET_RSI	0x18
#define IOFFSET_RDI	0x20
#define IOFFSET_R8	0x28
#define IOFFSET_R9	0x30
#define IOFFSET_R10	0x38
#define IOFFSET_R11	0x40
#define IOFFSET_RBP	0x48

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
.global memsetb
.global memsetw
.global memcpy
.global memcpy_from_uspace
.global memcpy_to_uspace
.global memcpy_from_uspace_failover_address
.global memcpy_to_uspace_failover_address

# Wrapper for generic memsetb
memsetb:
	jmp _memsetb

# Wrapper for generic memsetw
memsetw:
	jmp _memsetw

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
 * @retrun MEMCPY_DST on success, 0 on failure.
 */
memcpy:
memcpy_from_uspace:
memcpy_to_uspace:
	movq MEMCPY_DST, %rax

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

# Push all volatile general purpose registers on stack
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
	movq %rbp, IOFFSET_RBP(%rsp)
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
	movq IOFFSET_RBP(%rsp), %rbp
.endm

#define INTERRUPT_ALIGN 128
	
## Declare interrupt handlers
#
# Declare interrupt handlers for n interrupt
# vectors starting at vector i.
#
# The handlers call exc_dispatch().
#
.macro handler i n

	/*
	 * Choose between version with error code and version without error
	 * code. Both versions have to be of the same size. amd64 assembly is,
	 * however, a little bit tricky. For instance, subq $0x80, %rsp and
	 * subq $0x78, %rsp can result in two instructions with different
	 * op-code lengths.
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
	cld

	# Stop stack traces here
	xorq %rbp, %rbp

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

## Low-level syscall handler
# 
# Registers on entry:
#
# @param rcx		Userspace return address.
# @param r11		Userspace RLFAGS.
#
# @param rax		Syscall number.
# @param rdi		1st syscall argument.
# @param rsi		2nd syscall argument.
# @param rdx		3rd syscall argument.
# @param r10		4th syscall argument. Used instead of RCX because the
#			SYSCALL instruction clobbers it.
# @param r8		5th syscall argument.
# @param r9		6th syscall argument.
#
# @return		Return value is in rax.
#
syscall_entry:
	swapgs			# Switch to hidden gs	
	# 
	# %gs:0			Scratch space for this thread's user RSP
	# %gs:8			Address to be used as this thread's kernel RSP
	#
	movq %rsp, %gs:0	# Save this thread's user RSP
	movq %gs:8, %rsp	# Set this thread's kernel RSP
	swapgs			# Switch back to remain consistent
	sti
	
	pushq %rcx
	pushq %r11

	movq %r10, %rcx		# Copy the 4th argument where it is expected 
	pushq %rax
	call syscall_handler
	addq $8, %rsp
		
	popq %r11
	popq %rcx

	cli
	swapgs
	movq %gs:0, %rsp	# Restore the user RSP
	swapgs

	sysretq

.data
.global interrupt_handler_size

interrupt_handler_size: .quad (h_end-h_start)/IDT_ITEMS
