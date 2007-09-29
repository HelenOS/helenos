#
# Copyright (c) 2006 Martin Decky
# Copyright (c) 2006 Jakub Jermar
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

#include <stack.h>

.section BOOTSTRAP, "ax"

.global start
start:

	mov ar.rsc = r0
#	movl r8 = (VRN_KERNEL << VRN_SHIFT) ;;
	movl r1 = 0x4400000
	movl r8 = initial_stack ;;
	mov ar.bspstore = r8
	loadrs

	# initialize memory stack to some sane value
	movl r12 = initial_stack_top ;;
	
	add r12 = -16, r12	/* allocate a scratch area on the stack */

	# initialize gp (Global Pointer) register
	#movl r1 = _hardcoded_load_address

	ssm (1 << 19) ;; /* Disable f32 - f127 */
	srlz.i
	srlz.d ;;

	movl r18 = bootstrap ;;
	mov b1 = r18 ;;
	br.call.sptk.many b0 = b1


.section .bss, "ax"

.align 8192

initial_stack:
	.space 8192

.align 8192
initial_stack_top:
