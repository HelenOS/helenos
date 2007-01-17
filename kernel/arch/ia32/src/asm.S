#
# Copyright (c) 2001-2004 Jakub Jermar
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

## very low and hardware-level functions

# Mask for interrupts 0 - 31 (bits 0 - 31) where 0 means that int has no error word
# and 1 means interrupt with error word
#define ERROR_WORD_INTERRUPT_LIST 0x00027D00

.text

.global paging_on
.global enable_l_apic_in_msr
.global interrupt_handlers
.global memcpy
.global memcpy_from_uspace
.global memcpy_from_uspace_failover_address
.global memcpy_to_uspace
.global memcpy_to_uspace_failover_address


#define MEMCPY_DST	4
#define MEMCPY_SRC	8
#define MEMCPY_SIZE	12

/** Copy memory to/from userspace.
 *
 * This is almost conventional memcpy().
 * The difference is that there is a failover part
 * to where control is returned from a page fault
 * if the page fault occurs during copy_from_uspace()
 * or copy_to_uspace().
 *
 * @param MEMCPY_DST(%esp)	Destination address.
 * @param MEMCPY_SRC(%esp)	Source address.
 * @param MEMCPY_SIZE(%esp)	Size.
 *
 * @return MEMCPY_SRC(%esp) on success and 0 on failure.
 */
memcpy:
memcpy_from_uspace:
memcpy_to_uspace:
	movl %edi, %edx				/* save %edi */
	movl %esi, %eax				/* save %esi */
	
	movl MEMCPY_SIZE(%esp), %ecx
	shrl $2, %ecx				/* size / 4 */
	
	movl MEMCPY_DST(%esp), %edi
	movl MEMCPY_SRC(%esp), %esi
	
	rep movsl				/* copy as much as possible word by word */

	movl MEMCPY_SIZE(%esp), %ecx
	andl $3, %ecx				/* size % 4 */
	jz 0f
	
	rep movsb				/* copy the rest byte by byte */

0:
	movl %edx, %edi
	movl %eax, %esi
	movl MEMCPY_SRC(%esp), %eax		/* MEMCPY_SRC(%esp), success */
	ret
	
/*
 * We got here from as_page_fault() after the memory operations
 * above had caused a page fault.
 */
memcpy_from_uspace_failover_address:
memcpy_to_uspace_failover_address:
	movl %edx, %edi
	movl %eax, %esi
	xorl %eax, %eax				/* return 0, failure */
	ret

## Turn paging on
#
# Enable paging and write-back caching in CR0.
#
paging_on:
	movl %cr0,%edx
	orl $(1<<31),%edx		# paging on
	andl $~((1<<30)|(1<<29)),%edx	# clear Cache Disable and not Write Though
	movl %edx,%cr0
	jmp 0f
0:
	ret


## Enable local APIC
#
# Enable local APIC in MSR.
#
enable_l_apic_in_msr:
	push %eax

	movl $0x1b, %ecx
	rdmsr
	orl $(1<<11),%eax
	orl $(0xfee00000),%eax
	wrmsr

	pop %eax
	ret

# Clear nested flag
# overwrites %ecx
.macro CLEAR_NT_FLAG
	pushfl
	pop %ecx
	and $0xffffbfff,%ecx
	push %ecx
	popfl
.endm	

## Declare interrupt handlers
#
# Declare interrupt handlers for n interrupt
# vectors starting at vector i.
#
# The handlers setup data segment registers
# and call exc_dispatch().
#
#define INTERRUPT_ALIGN 64
.macro handler i n

.ifeq \i-0x30     # Syscall handler
	push %ds
	push %es
	push %fs
	push %gs

	# Push arguments on stack
	push %edi
	push %esi
	push %edx
	push %ecx
	push %eax
	
	# we must fill the data segment registers
	movw $16,%ax
	movw %ax,%ds
	movw %ax,%es
	
	sti
	
	call syscall_handler   # syscall_handler(ax,cx,dx,si,di)
	cli
	addl $20, %esp         # clean-up of parameters
	
	pop %gs
	pop %fs
	pop %es
	pop %ds
	
	CLEAR_NT_FLAG
	iret
.else	
	/*
	 * This macro distinguishes between two versions of ia32 exceptions.
	 * One version has error word and the other does not have it.
	 * The latter version fakes the error word on the stack so that the
	 * handlers and istate_t can be the same for both types.
	 */
	.iflt \i-32
		.if (1 << \i) & ERROR_WORD_INTERRUPT_LIST
			/* 
			 * With error word, do nothing
			 */
                .else
                        /*
                         * Version without error word,
                         */
			subl $4, %esp
                .endif
        .else
                /*
                 * Version without error word,
                 */
		subl $4, %esp
	.endif
	
	push %ds
	push %es
	push %fs
	push %gs

#ifdef CONFIG_DEBUG_ALLREGS
	push %ebx
	push %ebp
	push %edi
	push %esi
#else
	sub $16, %esp
#endif
	push %edx
	push %ecx
	push %eax
	
	# we must fill the data segment registers
	movw $16,%ax
	movw %ax,%ds
	movw %ax,%es

	pushl %esp          # *istate
	pushl $(\i)         # intnum
	call exc_dispatch   # excdispatch(intnum, *istate)
	addl $8,%esp        # Clear arguments from stack

	CLEAR_NT_FLAG # Modifies %ecx
	
	pop %eax
	pop %ecx
	pop %edx
#ifdef CONFIG_DEBUG_ALLREGS
	pop %esi
	pop %edi
	pop %ebp
	pop %ebx
#else
	add $16, %esp
#endif	
	
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4,%esp	# Skip error word, no matter whether real or fake.
	iret
.endif

	.align INTERRUPT_ALIGN
	.if (\n-\i)-1
	handler "(\i+1)",\n
	.endif
.endm

# keep in sync with pm.h !!!
IDT_ITEMS=64
.align INTERRUPT_ALIGN
interrupt_handlers:
h_start:
	handler 0 IDT_ITEMS
h_end:

.data
.global interrupt_handler_size

interrupt_handler_size: .long (h_end-h_start)/IDT_ITEMS
