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

.global memcpy
memcpy:
	br _memcpy

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
