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

.text

/** Copy memory from/to userspace.
 *
 * This memcpy() has been taken from the assembler output of
 * the generic _memcpy() and modified to have the failover part.
 *
 * @param in0 Destination address.
 * @param in1 Source address.
 * @param in2 Number of byte to copy.
 */
.global memcpy
.global memcpy_from_uspace
.global memcpy_to_uspace
.global memcpy_from_uspace_failover_address
.global memcpy_to_uspace_failover_address
memcpy:
memcpy_from_uspace:
memcpy_to_uspace:
	alloc loc0 = ar.pfs, 3, 1, 0, 0

	shr.u r18 = in2, 3
	mov r8 = in1 ;;			/* prepare to return in1 on success */
	cmp.ne p6, p7 = 0, r18	;;
(p7)	mov r17 = r0
(p7)	br.cond.dptk 1f ;;
	mov r16 = r0
	mov r17 = r0 ;;

0:
	shladd r14 = r17, 3, r0
	adds r16 = 1, r16 ;;
	add r15 = r14, r8
	sxt4 r17 = r16
	add r14 = r14, in0 ;;
	ld8 r15 = [r15]
	cmp.gtu p6, p7 = r18, r17 ;;
	st8 [r14] = r15
(p6)	br.cond.dptk 0b

1:
	and in2 = 7, in2 ;;
	cmp.eq p6, p7 = 0, in2 ;;
(p6)	mov ar.pfs = loc0
(p6)	br.ret.dptk.many rp
	shladd r14 = r17, 3, r0
	mov r16 = r0
	mov r17 = r0 ;;
	add in0 = in0, r14
	add r18 = r8, r14 ;;

2:
	add r14 = r16, r18
	adds r17 = 1, r17
	add r15 = in0, r16 ;;
	ld1 r14 = [r14]
	sxt4 r16 = r17	;;
	st1 [r15] = r14
	cmp.gtu p6, p7 = in2, r16
(p6)	br.cond.dptk 2b

	mov ar.pfs = loc0
	br.ret.sptk.many rp
	
memcpy_from_uspace_failover_address:
memcpy_to_uspace_failover_address:
	mov r8 = r0			/* return 0 on failure */
	mov ar.pfs = loc0
	br.ret.sptk.many rp

.global memsetb
memsetb:
	br _memsetb

.global cpu_halt
cpu_halt:
	br cpu_halt

.global panic_printf
panic_printf:
	{
		br.call.sptk.many b0=printf
	}
	br halt

/** Switch to userspace - low level code.
 *
 * @param in0 Userspace entry point address.
 * @param in1 Userspace stack pointer address.
 * @param in2 Userspace register stack pointer address.
 * @param in3 Userspace address of thread uspace_arg_t structure.
 * @param in4 Value to be stored in IPSR.
 * @param in5 Value to be stored in RSC.
 */
.global switch_to_userspace
switch_to_userspace:
	alloc loc0 = ar.pfs, 6, 3, 0, 0
	rsm (PSR_IC_MASK | PSR_I_MASK)		/* disable interruption collection and interrupts */
	srlz.d ;;
	srlz.i ;;
	
	mov cr.ipsr = in4
	mov cr.iip = in0
	mov r12 = in1

	xor r1 = r1, r1
	
	mov loc1 = cr.ifs
	movl loc2 = PFM_MASK ;;
	and loc1 = loc2, loc1 ;;
	mov cr.ifs = loc1 ;;			/* prevent decrementing BSP by rfi */

	invala
	
	mov loc1 = ar.rsc ;;
	and loc1 = ~3, loc1 ;;			
	mov ar.rsc = loc1 ;;			/* put RSE into enforced lazy mode */

	flushrs ;;
	
	mov ar.bspstore = in2 ;;
	mov ar.rsc = in5 ;;
	
	mov r8 = in3
	
	rfi ;;
