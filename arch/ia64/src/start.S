#
# Copyright (C) 2005 Jakub Jermar
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

#include <arch/register.h>
#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <mm/asid.h>

#define RR_MASK (0xFFFFFFFF00000002)
#define RID_SHIFT 8
#define PS_SHIFT 2

#define KERNEL_TRANSLATION_I 0x0010000000000661
#define KERNEL_TRANSLATION_D 0x0010000000000661

.section K_TEXT_START

.global kernel_image_start

stack0:
kernel_image_start:
	.auto

	# Fill TR.i and TR.d using Region Register #VRN_KERNEL

	movl r8=(VRN_KERNEL<<VRN_SHIFT)
	mov r9=rr[r8]
	movl r10=(RR_MASK)
	and r9=r10,r9
	movl r10=((RID_KERNEL<<RID_SHIFT)|(KERNEL_PAGE_WIDTH<<PS_SHIFT))
	or  r9=r10,r9
	mov rr[r8]=r9

	movl r8=(VRN_KERNEL<<VRN_SHIFT)
	mov cr.ifa=r8
	movl r10=(KERNEL_PAGE_WIDTH<<PS_SHIFT)
	mov cr.itir=r10
	movl r10=(KERNEL_TRANSLATION_I)
	itr.i itr[r0]=r10
	movl r10=(KERNEL_TRANSLATION_D)
	itr.d dtr[r0]=r10

	# initialize PSR
	mov psr.l = r0
	srlz.i
	srlz.d
	movl r10=(PSR_DT_MASK|PSR_RT_MASK|PSR_IT_MASK|PSR_IC_MASK)  /* Enable paging */
	mov r9=psr
	or r10=r10,r9
	mov cr.ipsr=r10
	mov cr.ifs=r0
	movl r8=paging_start
	mov cr.iip=r8
	srlz.d
	srlz.i

	.explicit
	/*
	 * Return From Interupt is the only the way to fill upper half word of PSR.
	 */
	rfi;;
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}

.global paging_start
paging_start:

	/*
	 * Now we are paging.
	 */

	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}
	{nop 0;;}

	# switch to register bank 1
	bsw.1
	
	# initialize register stack
	mov ar.rsc = r0
	movl r8=(VRN_KERNEL<<VRN_SHIFT) ;;
	mov ar.bspstore = r8
	loadrs

	# initialize memory stack to some sane value
	movl r12 = stack0;;
	
	add r12 = - 16, r12	/* allocate a scratch area on the stack */

	# initialize gp (Global Pointer) register
	movl r1 = _hardcoded_load_address	;;

	/*
	 * Initialize hardcoded_* variables.
	 */
	movl r14 = _hardcoded_ktext_size
	movl r15 = _hardcoded_kdata_size
	movl r16 = _hardcoded_load_address
	addl r17 = @gprel(hardcoded_ktext_size), gp
	addl r18 = @gprel(hardcoded_kdata_size), gp
	addl r19 = @gprel(hardcoded_load_address), gp
	;;
	st8 [r17] = r14
	st8 [r18] = r15
	st8 [r19] = r16

	movl r18=main_bsp ;;
	mov b1=r18 ;;
	br.call.sptk.many b0=b1

0:
	br 0b
