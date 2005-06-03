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

.section K_TEXT_START

.global kernel_image_start

stack0:
kernel_image_start:
	.auto
	# initialize PSR
	mov psr.l = r0
	
	# initialize register stack
	mov ar.rsc = r0
	mov ar.bspstore = r0
	loadrs

	.explicit
	# initialize memory stack to some sane value
	movl r12 = stack0	;;
	add r12 = - 16, r12	/* allocate a scratch area on the stack */

	# initialize gp (Global Pointer) register
	movl r1 = _hardcoded_load_address
	
	;;

	#
	# Initialize hardcoded_* variables.
	#
	movl r14 = _hardcoded_ktext_size
	movl r15 = _hardcoded_kdata_size
	movl r16 = _hardcoded_load_address
	addl r17 = @gprel(hardcoded_ktext_size), gp
	addl r18 = @gprel(hardcoded_kdata_size), gp
	addl r19 = @gprel(hardcoded_load_address), gp
	;;
	st4 [r17] = r14
	st4 [r18] = r15
	st8 [r19] = r16
	
	br.call.sptk.many b0=main_bsp

0:
	br 0b
