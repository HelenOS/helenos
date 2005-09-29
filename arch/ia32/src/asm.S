#
# Copyright (C) 2001-2004 Jakub Jermar
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

#  Mask for interrupts 0 - 31 (bits 0 - 31) where 0 means that int has no error word
# and 1 means interrupt with error word
#define ERROR_WORD_INTERRUPT_LIST 0x00027D00

.text

.global paging_on
.global enable_l_apic_in_msr
.global interrupt_handlers
.global memsetb
.global memsetw
.global memcmp


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


## Declare interrupt handlers
#
# Declare interrupt handlers for n interrupt
# vectors starting at vector i.
#
# The handlers setup data segment registers
# and call trap_dispatcher().
#
.macro handler i n
	push %ebp
	movl %esp,%ebp
	pusha

	push %ds
	push %es

	# we must fill the data segment registers
	movw $16,%ax
	movw %ax,%ds
	movw %ax,%es

	movl $(\i),%edi
	pushl %ebp
	addl $4,(%esp)
	pushl %edi
	call trap_dispatcher
	addl $8,%esp

	pop %es
	pop %ds


# CLNT
	pushfl
	pop %eax
	and $0xFFFFBFFF,%eax
	push %eax
	popfl
	


# Test if this is interrupt with error word or not
	mov $\i,%cl
	movl $1,%eax
	test $0xe0,%cl
	jnz 0f
	and $0x1f,%cl
	shl %cl,%eax
	and $ERROR_WORD_INTERRUPT_LIST,%eax
	jz 0f


# Return with error word
	popa
	pop %ebp
	add $4,%esp	# Skip error word
	iret

0:
# Return with no error word
	popa
	pop %ebp
	iret

	.if (\n-\i)-1
	handler "(\i+1)",\n
	.endif
.endm

# keep in sync with pm.h !!!
IDT_ITEMS=64
interrupt_handlers:
h_start:
	handler 0 64
#	handler 64 128	
#	handler 128 192
#	handler 192 256
h_end:


## Fill memory with bytes
#
# Fill a given number of bytes (2nd argument)
# at memory defined by 1st argument with the
# byte value defined by 3rd argument.
#
DST=12
CNT=16
X=20
memsetb:
	push %eax
	push %edi

	movl CNT(%esp),%ecx
	movl DST(%esp),%edi
	movl X(%esp),%eax

	rep stosb %al,%es:(%edi)

	pop %edi
	pop %eax
	ret


## Fill memory with words
#
# Fill a given number of words (2nd argument)
# at memory defined by 1st argument with the
# word value defined by 3rd argument.
#
DST=12
CNT=16
X=20
memsetw:
	push %eax
	push %edi

	movl CNT(%esp),%ecx
	movl DST(%esp),%edi
	movl X(%esp),%eax

	rep stosw %ax,%es:(%edi)

	pop %edi
	pop %eax

	ret


## Compare memory regions for equality
#
# Compare a given number of bytes (3rd argument)
# at memory locations defined by 1st and 2nd argument
# for equality. If the bytes are equal, EAX contains 0.
#
SRC=12
DST=16
CNT=20
memcmp:
	push %esi
	push %edi

	movl CNT(%esp),%ecx
	movl DST(%esp),%edi
	movl SRC(%esp),%esi

	repe cmpsb %es:(%edi),%ds:(%esi)
	movl %ecx,%eax		# %ecx contains the return value (zero on success)

	pop %edi
	pop %esi
	
	ret


# THIS IS USERSPACE CODE
.global utext
utext:
	xor %ax,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
0:
	int $48
	jmp 0b
	# not reached
utext_end:

.data
.global utext_size
utext_size:
	.long utext_end - utext 

.global interrupt_handler_size

interrupt_handler_size: .long (h_end-h_start)/IDT_ITEMS
