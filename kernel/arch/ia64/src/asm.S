/*
 * Copyright (c) 2005 Jakub Jermar
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

#include <abi/asmtool.h>
#include <arch/register.h>

.text

/** Copy memory from/to userspace.
 *
 * This memcpy() has been taken from the assembler output of
 * a plain C implementation of memcpy() modified to have the
 * failover part.
 *
 * @param in0 Destination address.
 * @param in1 Source address.
 * @param in2 Number of byte to copy.
 *
 */
FUNCTION_BEGIN(memcpy_from_uspace)
FUNCTION_BEGIN(memcpy_to_uspace)
	alloc loc0 = ar.pfs, 3, 1, 0, 0

	adds r14 = 7, in1
	mov r2 = ar.lc
	mov r8 = in0 ;;
	and r14 = -8, r14 ;;
	cmp.ne p6, p7 = r14, in1
	(p7) br.cond.dpnt 3f ;;

	0:

		cmp.ne p6, p7 = 0, in2
		(p7) br.cond.dpnt 2f ;;
		(p6) adds r14 = -1, in2
		(p6) mov r16 = r0
		(p6) mov r17 = r0 ;;
		(p6) mov ar.lc = r14

	1:

		add r14 = r16, in1
		add r15 = r16, in0
		adds r17 = 1, r17 ;;
		ld1 r14 = [r14]
		mov r16 = r17 ;;
		st1 [r15] = r14
		br.cloop.sptk.few 1b ;;

	2:

		mov ar.lc = r2
		mov ar.pfs = loc0
		br.ret.sptk.many rp

	3:

		adds r14 = 7, in0 ;;
		and r14 = -8, r14 ;;
		cmp.eq p6, p7 = r14, in0
		(p7) br.cond.dptk 0b
		shr.u r18 = in2, 3 ;;
		cmp.ne p6, p7 = 0, r18
		(p7) br.cond.dpnt 5f ;;
		(p6) adds r14 = -1, r18
		(p6) mov r16 = r0
		(p6) mov r17 = r0 ;;
		(p6) mov ar.lc = r14

	4:

		shladd r14 = r16, 3, r0
		adds r16 = 1, r17 ;;
		add r15 = in1, r14
		add r14 = in0, r14
		mov r17 = r16 ;;
		ld8 r15 = [r15] ;;
		st8 [r14] = r15
		br.cloop.sptk.few 4b

	5:

		and r15 = 7, in2
		shladd r14 = r18, 3, r0
		mov r16 = r0
		mov r18 = r0 ;;
		cmp.eq p6, p7 = 0, r15
		add in0 = r14, in0
		adds r15 = -1, r15
		add r17 = r14, in1
		(p6) br.cond.dpnt 2b ;;
		mov ar.lc = r15

	6:

		add r14 = r16, r17
		add r15 = r16, in0
		adds r16 = 1, r18 ;;
		ld1 r14 = [r14]
		mov r18 = r16 ;;
		st1 [r15] = r14
		br.cloop.sptk.few 6b ;;
		mov ar.lc = r2
		mov ar.pfs = loc0
		br.ret.sptk.many rp
FUNCTION_END(memcpy_from_uspace)
FUNCTION_END(memcpy_to_uspace)

SYMBOL(memcpy_from_uspace_failover_address)
SYMBOL(memcpy_to_uspace_failover_address)
	/* Return 0 on failure */
	mov r8 = r0
	mov ar.pfs = loc0
	br.ret.sptk.many rp

FUNCTION_BEGIN(cpu_halt)
	br cpu_halt
FUNCTION_END(cpu_halt)

/** Switch to userspace - low level code.
 *
 * @param in0 Userspace entry point address.
 * @param in1 Userspace stack pointer address.
 * @param in2 Userspace register stack pointer address.
 * @param in3 Userspace address of thread uspace_arg_t structure.
 * @param in4 Value to be stored in IPSR.
 * @param in5 Value to be stored in RSC.
 *
 */
FUNCTION_BEGIN(switch_to_userspace)
	alloc loc0 = ar.pfs, 6, 3, 0, 0

	/* Disable interruption collection and interrupts */
	rsm (PSR_IC_MASK | PSR_I_MASK)
	srlz.d ;;
	srlz.i ;;

	mov cr.ipsr = in4
	mov cr.iip = in0
	mov r12 = in1

	xor r1 = r1, r1

	/* r2 is defined to hold pcb_ptr - set it to 0 */
	xor r2 = r2, r2

	mov loc1 = cr.ifs
	movl loc2 = PFM_MASK ;;
	and loc1 = loc2, loc1 ;;
	mov cr.ifs = loc1 ;;  /* prevent decrementing BSP by rfi */

	invala

	mov loc1 = ar.rsc ;;
	and loc1 = ~3, loc1 ;;
	mov ar.rsc = loc1 ;;  /* put RSE into enforced lazy mode */

	flushrs ;;

	mov ar.bspstore = in2 ;;
	mov ar.rsc = in5 ;;

	mov r8 = in3

	rfi ;;
FUNCTION_END(switch_to_userspace)

FUNCTION_BEGIN(early_putuchar)
	br.ret.sptk.many b0
FUNCTION_END(early_putuchar)
