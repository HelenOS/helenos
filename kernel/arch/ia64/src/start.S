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

#include <arch/register.h>
#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <mm/asid.h>

#define RR_MASK (0xFFFFFFFF00000002)
#define RID_SHIFT	8
#define PS_SHIFT	2

#define KERNEL_TRANSLATION_I	0x0010000000000661
#define KERNEL_TRANSLATION_D	0x0010000000000661
#define KERNEL_TRANSLATION_VIO	0x0010000000000671
#define KERNEL_TRANSLATION_IO	0x00100FFFFC000671 
#define KERNEL_TRANSLATION_FW	0x00100000F0000671 

.section K_TEXT_START, "ax"

.global kernel_image_start

stack0:
kernel_image_start:
	.auto

	# Identify self(CPU) in OS structures by ID / EID

	mov r9 = cr64
	mov r10 = 1
	movl r12 = 0xffffffff
	movl r8 = cpu_by_id_eid_list
	and r8 = r8, r12
	shr r9 = r9, 16
	add r8 = r8, r9
	st1 [r8] = r10

	mov psr.l = r0
	srlz.i
	srlz.d

	# Fill TR.i and TR.d using Region Register #VRN_KERNEL

	movl r8 = (VRN_KERNEL << VRN_SHIFT)
	mov r9 = rr[r8]

	movl r10 = (RR_MASK)
	and r9 = r10, r9
	movl r10 = ((RID_KERNEL << RID_SHIFT) | (KERNEL_PAGE_WIDTH << PS_SHIFT))
	or  r9 = r10, r9

	mov rr[r8] = r9

	movl r8 = (VRN_KERNEL << VRN_SHIFT)
	mov cr.ifa = r8

	mov r11 = cr.itir ;;
	movl r10 = (KERNEL_PAGE_WIDTH << PS_SHIFT);;
	or r10 = r10, r11 ;;
	mov cr.itir = r10;;

	movl r10 = (KERNEL_TRANSLATION_I)
	itr.i itr[r0] = r10
	movl r10 = (KERNEL_TRANSLATION_D)
	itr.d dtr[r0] = r10

	movl r7 = 1
	movl r8 = (VRN_KERNEL << VRN_SHIFT) | VIO_OFFSET
	mov cr.ifa = r8
	movl r10 = (KERNEL_TRANSLATION_VIO)
	itr.d dtr[r7] = r10

	mov r11 = cr.itir ;;
	movl r10 = ~0xfc;;
	and r10 = r10, r11 ;;
	movl r11 = (IO_PAGE_WIDTH << PS_SHIFT);;
	or r10 = r10, r11 ;;
	mov cr.itir = r10;;

	movl r7 = 2
	movl r8 = (VRN_KERNEL << VRN_SHIFT) | IO_OFFSET
	mov cr.ifa = r8
	movl r10 = (KERNEL_TRANSLATION_IO)
	itr.d dtr[r7] = r10

	# Setup mapping for fimware arrea (also SAPIC)

	mov r11 = cr.itir ;;
	movl r10 = ~0xfc;;
	and r10 = r10, r11 ;;
	movl r11 = (FW_PAGE_WIDTH << PS_SHIFT);;
	or r10 = r10, r11 ;;
	mov cr.itir = r10;;

	movl r7 = 3
	movl r8 = (VRN_KERNEL << VRN_SHIFT) | FW_OFFSET
	mov cr.ifa = r8
	movl r10 = (KERNEL_TRANSLATION_FW)
	itr.d dtr[r7] = r10

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
	 * Return From Interupt is the only the way to fill upper half word of PSR.
	 */
	rfi;;


.global paging_start
paging_start:

	/*
	 * Now we are paging.
	 */

	# Switch to register bank 1
	bsw.1

	# Am I BSP or AP?
	movl r20 = bsp_started;;
	ld8 r20 = [r20];;
	cmp.eq p3, p2 = r20, r0;;
	
	# Initialize register stack
	mov ar.rsc = r0
	movl r8 = (VRN_KERNEL << VRN_SHIFT) ;;
	mov ar.bspstore = r8
	loadrs

	# Initialize memory stack to some sane value
	movl r12 = stack0 ;;
	add r12 = -16, r12	/* allocate a scratch area on the stack */

	# Initialize gp (Global Pointer) register
	movl r20 = (VRN_KERNEL << VRN_SHIFT);;
	or r20 = r20,r1;;
	movl r1 = _hardcoded_load_address
	
	/*
	 * Initialize hardcoded_* variables. Do only BSP
	 */
(p3)	movl r14 = _hardcoded_ktext_size
(p3)	movl r15 = _hardcoded_kdata_size
(p3)	movl r16 = _hardcoded_load_address ;;
(p3)	addl r17 = @gprel(hardcoded_ktext_size), gp
(p3)	addl r18 = @gprel(hardcoded_kdata_size), gp
(p3)	addl r19 = @gprel(hardcoded_load_address), gp
(p3)	addl r21 = @gprel(bootinfo), gp
	;;
(p3)	st8 [r17] = r14
(p3)	st8 [r18] = r15
(p3)	st8 [r19] = r16
(p3)	st8 [r21] = r20

	ssm (1 << 19) ;; /* Disable f32 - f127 */
	srlz.i
	srlz.d ;;

(p2)	movl r18 = main_ap ;;
(p2)   	mov b1 = r18 ;;
(p2)	br.call.sptk.many b0 = b1

	# Mark that BSP is on
	mov r20=1;;
	movl r21=bsp_started;;
	st8 [r21]=r20;;

	br.call.sptk.many b0 = arch_pre_main

	movl r18 = main_bsp ;;
	mov b1 = r18 ;;
	br.call.sptk.many b0 = b1

0:
	br 0b
.align 4096

kernel_image_ap_start:
	.auto

	# Identify self(CPU) in OS structures by ID / EID

	mov r9 = cr64
	mov r10 = 1
	movl r12 = 0xffffffff
	movl r8 = cpu_by_id_eid_list
	and r8 = r8, r12
	shr r9 = r9, 16
	add r8 = r8, r9
	st1 [r8] = r10
	
	# Wait for wakeup synchro signal (#3 in cpu_by_id_eid_list)
kernel_image_ap_start_loop:
	movl r11 = kernel_image_ap_start_loop
	and r11 = r11, r12
   	mov b1 = r11 

	ld1 r20 = [r8];;
	movl r21 = 3;;
	cmp.eq p2, p3 = r20, r21;;
(p3)	br.call.sptk.many b0 = b1

	movl r11 = kernel_image_start
	and r11 = r11, r12
	mov b1 = r11 
	br.call.sptk.many b0 = b1


.align 16
.global bsp_started
bsp_started:
.space 8

.align 4096
.global cpu_by_id_eid_list
cpu_by_id_eid_list:
.space 65536

