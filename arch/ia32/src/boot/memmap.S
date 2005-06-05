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


#include <arch/boot/memmap.h>

E820_RECORD_SIZE = MEMMAP_E820_RECORD_SIZE
E820_MAX_RECORDS = MEMMAP_E820_MAX_RECORDS
E820_SMAP = 0x534d4150

.global memmap_arch_init

.code16
.section K_TEXT_START_2

memmap_arch_init:
	
e820begin:
	xorl	%ebx,%ebx	# during first call, ebx must be 0
	
	movw	$e820table,%di
	movb	$E820_MAX_RECORDS,(e820counter)
e820loop:	
	movl	$E820_SMAP,%edx 	# constrol sequence "SMAP"
	pushw	%ds
	popw	%es		# e820 buffer address

	movl	$0xe820,%eax		# service
	movl	$E820_RECORD_SIZE,%ecx
	int 	$0x15
	jc		e820end	
	
	
	cmpl	$E820_SMAP,%eax		# verifying BIOS
	jne		e820end
	
	movw	%di,%ax		# next record
	addw	$E820_RECORD_SIZE,%ax
	movw	%ax,%di
		
	decb	(e820counter) # buffer is full
	jz	e820end
	
	cmpl	$0,%ebx	
	jne	e820loop
	
e820end:
	
	movb	(e820counter),%al	
	negb	%al
	addb	$32,%al
	movb	%al,(e820counter) # store # of valid entries in e820counter
	
	ret


e820counter:
	.byte 0
		 
e820table:
	.space  (32*E820_RECORD_SIZE),0 # space for 32 records, each E820_RECORD_SIZE bytes long
