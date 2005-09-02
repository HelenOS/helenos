/*
 * Copyright (C) 2005 Josef Cejka
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


#include <arch/boot/memmapasm.h>

E820_RECORD_SIZE = MEMMAP_E820_RECORD_SIZE
E820_MAX_RECORDS = MEMMAP_E820_MAX_RECORDS
E820_SMAP = 0x534d4150

.global memmap_arch_init
.global e801memorysize

.code16
.section K_TEXT_START_2

memmap_arch_init:

e820begin:
	xorl	%ebx,%ebx	# during first call, ebx must be 0
	movw	%bx,%ds
	movw	%bx,%es
	movw	$e820table_boot,%di
	movb	$E820_MAX_RECORDS,e820counter_boot
e820loop:	
	movl	$E820_SMAP,%edx 	# control sequence "SMAP"

	movl	$0x0000e820,%eax	# service
	movl	$E820_RECORD_SIZE,%ecx
	int 	$0x15
	jc	e820err
	
	cmpl	$E820_SMAP,%eax		# verifying BIOS
	jne	e820err

	cmpl	$E820_RECORD_SIZE,%ecx
	jne	e820err			# bad record size - bug in bios
	
	movw	%di,%ax		# next record
	addw	$E820_RECORD_SIZE,%ax
	movw	%ax,%di
		
	decb	e820counter_boot # buffer is full
	jz	e820end
	
	cmpl	$0,%ebx	
	jne	e820loop
	
e820end:
	movb	$E820_MAX_RECORDS,%al
	subb	e820counter_boot,%al
	movb	%al,e820counter_boot # store # of valid entries in e820counter

	jmp	e801begin

e820err:
	movb	$0,e820counter_boot

# method e801 - get size of memory

e801begin:
	xorw	%dx,%dx
	xorw	%cx,%cx
	xorw	%bx,%bx
	movw	$0xe801,%ax
	stc
	int	$0x15
	
	jc	e801end
	
			# fix problem with some BIOSes which use ax:bx rather than cx:dx
	testw	%cx,%cx
	jnz	e801cxdx
	testw	%dx,%dx
	jnz	e801cxdx

	movw	%ax,%cx
	movw	%bx,%dx
	
e801cxdx:
	andl	$0xffff,%edx
	shll	$6,%edx
	andl	$0xffff,%ecx
	addl	%ecx,%edx
	addl	$0x0400,%edx  # add lower 1 MB - it's not included by e801 method
	movl	%edx,e801memorysize
e801end:
	ret

	#memory size in 1 kb chunks
e801memorysize:
	.long	0
	