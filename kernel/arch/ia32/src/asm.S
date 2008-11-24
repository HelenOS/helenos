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

# Mask for interrupts 0 - 31 (bits 0 - 31) where 0 means that int has no error
# word and 1 means interrupt with error word
#define ERROR_WORD_INTERRUPT_LIST 0x00027d00

.text

.global paging_on
.global enable_l_apic_in_msr
.global interrupt_handlers
.global memsetb
.global memsetw
.global memcpy
.global memcpy_from_uspace
.global memcpy_from_uspace_failover_address
.global memcpy_to_uspace
.global memcpy_to_uspace_failover_address


# Wrapper for generic memsetb
memsetb:
	jmp _memsetb

# Wrapper for generic memsetw
memsetw:
	jmp _memsetw


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
 * @return MEMCPY_DST(%esp) on success and 0 on failure.
 */
memcpy:
memcpy_from_uspace:
memcpy_to_uspace:
	movl %edi, %edx			/* save %edi */
	movl %esi, %eax			/* save %esi */
	
	movl MEMCPY_SIZE(%esp), %ecx
	shrl $2, %ecx			/* size / 4 */
	
	movl MEMCPY_DST(%esp), %edi
	movl MEMCPY_SRC(%esp), %esi
	
	rep movsl			/* copy whole words */

	movl MEMCPY_SIZE(%esp), %ecx
	andl $3, %ecx			/* size % 4 */
	jz 0f
	
	rep movsb			/* copy the rest byte by byte */

0:
	movl %edx, %edi
	movl %eax, %esi
	movl MEMCPY_DST(%esp), %eax	/* MEMCPY_DST(%esp), success */
	ret
	
/*
 * We got here from as_page_fault() after the memory operations
 * above had caused a page fault.
 */
memcpy_from_uspace_failover_address:
memcpy_to_uspace_failover_address:
	movl %edx, %edi
	movl %eax, %esi
	xorl %eax, %eax			/* return 0, failure */
	ret

## Turn paging on
#
# Enable paging and write-back caching in CR0.
#
paging_on:
	movl %cr0, %edx
	orl $(1 << 31), %edx		# paging on
	# clear Cache Disable and not Write Though
	andl $~((1 << 30) | (1 << 29)), %edx
	movl %edx,%cr0
	jmp 0f
0:
	ret


## Enable local APIC
#
# Enable local APIC in MSR.
#
enable_l_apic_in_msr:
	movl $0x1b, %ecx
	rdmsr
	orl $(1 << 11), %eax
	orl $(0xfee00000), %eax
	wrmsr
	ret

# Clear nested flag
# overwrites %ecx
.macro CLEAR_NT_FLAG
	pushfl
	pop %ecx
	and $0xffffbfff, %ecx
	push %ecx
	popfl
.endm	

/*
 * The SYSENTER syscall mechanism can be used for syscalls with
 * four or fewer arguments. To pass these four arguments, we
 * use four registers: EDX, ECX, EBX, ESI. The syscall number
 * is passed in EAX. We use EDI to remember the return address
 * and EBP to remember the stack. The INT-based syscall mechanism
 * can actually handle six arguments plus the syscall number
 * entirely in registers.
 */
.global sysenter_handler
sysenter_handler:
	sti
	pushl %ebp	# remember user stack
	pushl %edi	# remember return user address

	pushl %gs	# remember TLS

	pushl %eax	# syscall number
	subl $8, %esp	# unused sixth and fifth argument
	pushl %esi	# fourth argument
	pushl %ebx	# third argument
	pushl %ecx	# second argument
	pushl %edx	# first argument

	movw $16, %ax
	movw %ax, %ds
	movw %ax, %es

	cld
	call syscall_handler
	addl $28, %esp	# remove arguments from stack

	pop %gs		# restore TLS

	pop %edx	# prepare return EIP for SYSEXIT
	pop %ecx	# prepare userspace ESP for SYSEXIT

	sysexit		# return to userspace


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

.ifeq \i - 0x30     # Syscall handler
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs

	#
	# Push syscall arguments onto the stack
	#
	# NOTE: The idea behind the order of arguments passed in registers is to
	#	use all scratch registers first and preserved registers next.
	#	An optimized libc syscall wrapper can make use of this setup.
	#
	pushl %eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %ebx
	pushl %ecx
	pushl %edx
	
	# we must fill the data segment registers
	movw $16, %ax
	movw %ax, %ds
	movw %ax, %es
	
	cld
	sti
	# syscall_handler(edx, ecx, ebx, esi, edi, ebp, eax)
	call syscall_handler   
	cli
	addl $28, %esp         # clean-up of parameters
	
	popl %gs
	popl %fs
	popl %es
	popl %ds
	
	CLEAR_NT_FLAG
	iret
.else	
	/*
	 * This macro distinguishes between two versions of ia32 exceptions.
	 * One version has error word and the other does not have it.
	 * The latter version fakes the error word on the stack so that the
	 * handlers and istate_t can be the same for both types.
	 */
	.iflt \i - 32
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
	
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs

#ifdef CONFIG_DEBUG_ALLREGS
	pushl %ebx
	pushl %ebp
	pushl %edi
	pushl %esi
#else
	subl $16, %esp
#endif
	pushl %edx
	pushl %ecx
	pushl %eax
	
	# we must fill the data segment registers
	movw $16, %ax
	movw %ax, %ds
	movw %ax, %es

	cld

	pushl %esp          # *istate
	pushl $(\i)         # intnum
	call exc_dispatch   # excdispatch(intnum, *istate)
	addl $8, %esp       # Clear arguments from stack

	CLEAR_NT_FLAG # Modifies %ecx
	
	popl %eax
	popl %ecx
	popl %edx
#ifdef CONFIG_DEBUG_ALLREGS
	popl %esi
	popl %edi
	popl %ebp
	popl %ebx
#else
	addl $16, %esp
#endif	
	
	popl %gs
	popl %fs
	popl %es
	popl %ds

	addl $4, %esp	# Skip error word, no matter whether real or fake.
	iret
.endif

	.align INTERRUPT_ALIGN
	.if (\n- \i) - 1
	handler "(\i + 1)", \n
	.endif
.endm

# keep in sync with pm.h !!!
IDT_ITEMS = 64
.align INTERRUPT_ALIGN
interrupt_handlers:
h_start:
	handler 0 IDT_ITEMS
h_end:

.data
.global interrupt_handler_size

interrupt_handler_size: .long (h_end - h_start) / IDT_ITEMS
