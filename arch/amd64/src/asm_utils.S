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


#  Mask for interrupts 0 - 31 (bits 0 - 31) where 0 means that int has no error word
# and 1 means interrupt with error word

	
#define ERROR_WORD_INTERRUPT_LIST 0x00027D00

#define __ASM__
#include <arch/pm.h>
	
.text
.global interrupt_handlers
.global panic_printf

panic_printf:
	movq $halt, (%rsp)
	jmp printf

.global memcpy
memcpy:
	jmp _memcpy
	
.global cpuid
.global has_cpuid
.global rdtsc
.global read_efer_flag
.global set_efer_flag
	

# THIS IS USERSPACE CODE
.global utext
utext:
0:
	int $48
	jmp 0b
	# not reached
utext_end:

.data
.global utext_size
utext_size:
	.long utext_end - utext 

	
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

rdtsc:
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
.macro push_all_gpr
	pushq %rax 
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rsi
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
.endm

.macro pop_all_gpr
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rdi
	popq %rsi
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
.endm
	
## Declare interrupt handlers
#
# Declare interrupt handlers for n interrupt
# vectors starting at vector i.
#
# The handlers setup data segment registers
# and call trap_dispatcher().
#
.macro handler i n
	pushq %rbp
	movq %rsp,%rbp
	
	push_all_gpr

	movq $(\i),%rdi   # %rdi - first parameter
	movq %rbp, %rsi
	addq $8, %rsi     # %rsi - second parameter - original stack
	call trap_dispatcher 	# trap_dispatcher(i, stack)

# Test if this is interrupt with error word or not
	mov $\i,%cl;
	movl $1,%eax;
	test $0xe0,%cl;
	jnz 0f;
	and $0x1f,%cl;
	shl %cl,%eax;
	and $ERROR_WORD_INTERRUPT_LIST,%eax;
	jz 0f;


# Return with error word
	pop_all_gpr
	
	popq %rbp;
	add $8,%esp;    # Skip error word
	iretq

0:
# Return with no error word
	pop_all_gpr
	
	popq %rbp
	iretq

	.if (\n-\i)-1
	handler "(\i+1)",\n
	.endif
.endm
	
interrupt_handlers:
h_start:
	handler 0 IDT_ITEMS
h_end:
	
	
.data
.global interrupt_handler_size

interrupt_handler_size: .long (h_end-h_start)/IDT_ITEMS
