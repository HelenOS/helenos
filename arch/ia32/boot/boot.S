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

# This is a very primitive boot.s

# It assumes that the first sector of the kernel image is found
# on head 0, track 0, sector 2 of the first floppy drive (1440K).
# Next kernel sectors follow on disk sector 3, 4, ...
#


.text
.global _start_0x7c00

.code16
_start_0x7c00:
	xorw %ax,%ax		# reset, %al will be used below
	movw %ax,%dx		# fd0, %dh and %dl will be used below
	int $0x13
	jc stop_trying

	movb $2,%ah		# read
	incb %al		# one sector
	movb %ah,%cl		# from disk sector 2
	movw %dx,%es
	movw $0x8000,%bx	# at %es:%bx
	int $0x13
	jc stop_trying

	pushw $('.')
	call echo_mark
	add $2,%sp
    
    	movw %dx,%ds
	
	# KERNEL_SIZE is passed from the outside to the preprocessor
	movl $(KERNEL_SIZE),%edi
	
	shrl $9,%edi		# number of sectors that kernel occupies (%edi div 512)
    
read_next:
	test %edi,%edi
	jnz read_sectors

	movb $12,%al
	movw $0x3f2,%dx
	outb %al,%dx

	pushw $('$')
	call echo_mark
	add $2,%sp	
	
	jmpl $0,$0x8000

read_sectors:
	pushw $('.')
	call echo_mark
	add $2,%sp

	decl %edi
	incw logical_sector
	movw %es,%si
	addw $0x20,%si
	movw %si,%es

	movw logical_sector,%ax
	divb sectors
    
	movb %ah,%cl
        incb %cl			# sector
    
	movb %al,%ch
	shrb $1,%ch			# track
    
	movb %al,%dh
	andb $1,%dh			# head

	movw $0x0201,%ax
	int $0x13
	jnc read_next

	pushw $('R')
	call echo_mark
	add $2,%sp

	xorw %ax,%ax			# try to reset
	movw %ax,%dx			# fd0
	int $0x13
	jnc read_next

stop_trying:
	pushw $('F')
	call echo_mark
	add $2,%sp

	cli
	hlt

CH=4
echo_mark:
	push %bp
	movw %sp,%bp
	pusha
    
	movw CH(%bp),%ax
	movb $0xe,%ah
	movb $7,%bl
	int $0x10
    
	popa
	pop %bp
	ret

# current logical sector from the beginning of the disk
logical_sector:
	.word 1

# number of sectors per track on 1440 floppy
sectors:
	.byte 18


# boot floppy signature
.org 0x1fe
boot_floppy_signature:
	.byte 0x55
	.byte 0xaa
