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

# very low and hardware-level functions

.text

.global cpu_priority_high
.global cpu_priority_low
.global cpu_priority_restore
.global cpu_priority_read
.global cpu_halt
.global cpu_sleep
.global paging_on
.global cpu_read_dba
.global cpu_write_dba
.global cpu_read_cr2
.global enable_l_apic_in_msr
.global interrupt_handlers
.global inb
.global inw
.global inl
.global outb
.global outw
.global outl
.global memcopy
.global memsetb
.global memsetw
.global memcmp

#
# set priority level high
cpu_priority_high:
	pushf
	pop %eax
	cli
	ret
    
#
# set priority level low
cpu_priority_low:
        pushf
	pop %eax
	sti
	ret

#
# restore priority level
cpu_priority_restore:
	push 4(%esp)
	popf
	ret

# return raw priority level
cpu_priority_read:
	pushf
	pop %eax
	ret

cpu_halt:
cpu_sleep:
	hlt
	ret

paging_on:
	pushl %eax
	movl %cr0,%eax
	orl $(1<<31),%eax		# paging on
	andl $~((1<<30)|(1<<29)),%eax	# clear Cache Disable and not Write Though
	movl %eax,%cr0
	jmp 0f
0:
	popl %eax
	ret

cpu_read_dba:
	movl %cr3,%eax
	ret

cpu_write_dba:
	pushl %eax
	movl 8(%esp),%eax
	movl %eax,%cr3
	popl %eax
	ret

cpu_read_cr2:
	movl %cr2,%eax
	ret

enable_l_apic_in_msr:
	pusha
	
	movl $0x1b, %ecx
	rdmsr
	orl $(1<<11),%eax
	orl $(0xfee00000),%eax
	wrmsr
	
	popa
	ret

.macro handler i n
	push %ebp
	movl %esp,%ebp
	pusha
    
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


inb:
	push %edx
	xorl %eax,%eax
	movl 8(%esp),%edx
	inb %dx,%al
	pop %edx
	ret

inw:
	push %edx
	xorl %eax,%eax
	movl 8(%esp),%edx
	inw %dx,%ax
	pop %edx
	ret

inl:
	push %edx
	xorl %eax,%eax
	movl 8(%esp),%edx
	inl %dx,%eax
	pop %edx
	ret

outb:
	push %ebp
	movl %esp,%ebp
	pusha
    
	movl 8(%ebp),%edx
	movl 12(%ebp),%eax
	outb %al,%dx
    
	popa
	pop %ebp
	ret

outw:
	push %ebp
	movl %esp,%ebp
	pusha
    
	movl 8(%ebp),%edx
	movl 12(%ebp),%eax
	outw %ax,%dx
    
	popa
	pop %ebp
	ret

outl:
	push %ebp
	movl %esp,%ebp
	pusha
    
	movl 8(%ebp),%edx
	movl 12(%ebp),%eax
	outl %eax,%dx
    
	popa
	pop %ebp
	ret

SRC=8
DST=12
CNT=16
memcopy:
	push %ebp
	movl %esp,%ebp
	pusha
    
	cld
	movl CNT(%ebp),%ecx
	movl DST(%ebp),%edi
	movl SRC(%ebp),%esi    
    
	rep movsb %ds:(%esi),%es:(%edi)
    
	popa
	pop %ebp
	ret

DST=8
CNT=12
X=16
memsetw:
	push %ebp
	movl %esp,%ebp
	pusha
    
	cld
	movl CNT(%ebp),%ecx
	movl DST(%ebp),%edi
	movl X(%ebp),%eax
    
	rep stosw %ax,%es:(%edi)
    
        popa
	pop %ebp
	ret

DST=8
CNT=12
X=16
memsetb:
	push %ebp
	movl %esp,%ebp
	pusha
    
	cld
	movl CNT(%ebp),%ecx
	movl DST(%ebp),%edi
	movl X(%ebp),%eax
    
	rep stosb %al,%es:(%edi)
    
        popa
	pop %ebp
	ret

SRC=12
DST=16
CNT=20
memcmp:
	push %ebp
	subl $4,%esp	
	movl %esp,%ebp

	pusha
    
	cld
	movl CNT(%ebp),%ecx
	movl DST(%ebp),%edi
	movl SRC(%ebp),%esi    
    
	repe cmpsb %es:(%edi),%ds:(%esi)
	movl %ecx,(%ebp)

	popa
	
	movl (%ebp),%eax	# return value => %eax (zero on success)
	addl $4,%esp
	pop %ebp
	
	ret


# THIS IS USERSPACE CODE
.global utext
utext:
0:
	movl $0xdeadbeaf, %eax
	int $48
	jmp 0b
	# not reached
utext_end:

.data
.global utext_size
utext_size:
	.long utext_end - utext 


.section K_DATA_START
.global interrupt_handler_size

interrupt_handler_size: .long (h_end-h_start)/IDT_ITEMS
