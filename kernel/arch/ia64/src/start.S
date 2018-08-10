#
# Copyright (c) 2005 Jakub Jermar
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

#include <abi/asmtool.h>
#include <arch/register.h>
#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <mm/asid.h>

#define RR_MASK    (0xFFFFFFFF00000002)
#define RID_SHIFT  8
#define PS_SHIFT   2

#define KERNEL_TRANSLATION_I    0x0010000000000661
#define KERNEL_TRANSLATION_D    0x0010000000000661

.section K_TEXT_START, "ax"

stack0:

#
# Kernel entry point.
#
# This is where we are passed control from the boot code.
# Register contents:
#
#	r2	Address of the boot code's bootinfo structure.
#
SYMBOL(kernel_image_start)
	.auto

	mov psr.l = r0
	srlz.i
	srlz.d

	# Fill TR.i and TR.d using Region Register #VRN_KERNEL

	movl r8 = (VRN_KERNEL << VRN_SHIFT)
	mov r9 = rr[r8]

	movl r10 = (RR_MASK)
	and r9 = r10, r9
	movl r10 = (((RID_KERNEL7) << RID_SHIFT) | (KERNEL_PAGE_WIDTH << PS_SHIFT))
	or r9 = r10, r9

	mov rr[r8] = r9

	movl r8 = (VRN_KERNEL << VRN_SHIFT)
	mov cr.ifa = r8

	mov r11 = cr.itir
	movl r10 = (KERNEL_PAGE_WIDTH << PS_SHIFT)
	or r10 = r10, r11
	mov cr.itir = r10

	movl r10 = (KERNEL_TRANSLATION_I)
	itr.i itr[r0] = r10
	movl r10 = (KERNEL_TRANSLATION_D)
	itr.d dtr[r0] = r10

	# Initialize DCR

	movl r10 = (DCR_DM_MASK | DCR_DP_MASK | DCR_DK_MASK | DCR_DX_MASK | DCR_DR_MASK | DCR_DA_MASK | DCR_DD_MASK | DCR_LC_MASK)
	mov r9 = cr.dcr
	or r10 = r10, r9
	mov cr.dcr = r10

	# Initialize PSR

	movl r10 = (PSR_DT_MASK | PSR_RT_MASK | PSR_IT_MASK | PSR_IC_MASK)  /* Enable paging */
	mov r9 = psr

	or r10 = r10, r9
	mov cr.ipsr = r10
	mov cr.ifs = r0
	movl r8 = paging_start
	mov cr.iip = r8
	srlz.d
	srlz.i

	.explicit

	/*
	 * Return From Interrupt is the only way to
	 * fill the upper half word of PSR.
	 */
	rfi ;;

paging_start:

	/*
	 * Now we are paging.
	 */

	#
	# Set Interruption Vector Address
	# (i.e. location of interruption vector table)
	#
	movl r8 = ivt ;;
	mov cr.iva = r8
	srlz.d ;;


	# Switch to register bank 1
	bsw.1

	# Initialize register stack
	mov ar.rsc = r0
	movl r8 = (VRN_KERNEL << VRN_SHIFT) ;;
	mov ar.bspstore = r8
	loadrs

	#
	# Initialize memory stack to some sane value and allocate a scratch area
	# on it.
	#
	movl sp = stack0 ;;
	add sp = -16, sp

	# Initialize gp (Global Pointer) register
	movl gp = __gp

	#
	# Initialize bootinfo on BSP.
	#
	movl r20 = (VRN_KERNEL << VRN_SHIFT) ;;
	or r20 = r20, r2 ;;
	addl r21 = @gprel(bootinfo), gp ;;
	st8 [r21] = r20

	ssm (1 << 19) ;; /* Disable f32 - f127 */
	srlz.i
	srlz.d ;;

	br.call.sptk.many b0 = ia64_pre_main
0:
	br.call.sptk.many b0 = main_bsp
0:
	br 0b
