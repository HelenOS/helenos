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

.global xen_callback
.global xen_failsafe_callback
.global enable_l_apic_in_msr
.global memcpy
.global memcpy_from_uspace
.global memcpy_from_uspace_failover_address
.global memcpy_to_uspace
.global memcpy_to_uspace_failover_address


xen_callback:
	iret

xen_failsafe_callback:
	iret


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
