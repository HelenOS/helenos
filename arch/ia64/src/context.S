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

.text

.global context_save
.global context_restore

context_save:
	alloc loc0 = ar.pfs, 1, 2, 0, 0
	
	/*
	 * TODO: save the rest of the context registers.
	 */

	st8 [in0] = r1, 8	;;
	st8 [in0] = r2, 8	;;
	st8 [in0] = r3, 8	;;
	st8 [in0] = r4, 8	;;
	st8 [in0] = r5, 8	;;
	st8 [in0] = r6, 8	;;
	st8 [in0] = r7, 8	;;
	st8 [in0] = r8, 8	;;
	st8 [in0] = r9, 8	;;
	st8 [in0] = r10, 8	;;
	st8 [in0] = r11, 8	;;
	st8 [in0] = r12, 8	;;	/* save sp */
	st8 [in0] = r13, 8	;;
	st8 [in0] = r14, 8	;;
	st8 [in0] = r15, 8	;;
	st8 [in0] = r16, 8	;;
	st8 [in0] = r17, 8	;;
	st8 [in0] = r18, 8	;;
	st8 [in0] = r19, 8	;;
	st8 [in0] = r20, 8	;;
	st8 [in0] = r21, 8	;;
	st8 [in0] = r22, 8	;;
	st8 [in0] = r23, 8	;;
	st8 [in0] = r24, 8	;;
	st8 [in0] = r25, 8	;;
	st8 [in0] = r26, 8	;;
	st8 [in0] = r27, 8	;;
	st8 [in0] = r28, 8	;;
	st8 [in0] = r29, 8	;;
	st8 [in0] = r30, 8	;;
	st8 [in0] = r31, 8	;;

	/* save pc */
	mov loc1 = b0		;;
	st8 [in0] = loc1, 8
	
	mov ar.pfs = loc0
	
	add r8 = r0, r0, 1 		/* context_save returns 1 */
	br.ret.sptk.many b0

context_restore:
	alloc loc0 = ar.pfs, 1, 2, 0, 0

	/*
	 * TODO: restore the rest of the context registers.
	 */
	
	ld8 r1 = [in0], 8	;;
	ld8 r2 = [in0], 8	;;
	ld8 r3 = [in0], 8	;;
	ld8 r4 = [in0], 8	;;
	ld8 r5 = [in0], 8	;;
	ld8 r6 = [in0], 8	;;
	ld8 r7 = [in0], 8	;;
	ld8 r8 = [in0], 8	;;
	ld8 r9 = [in0], 8	;;
	ld8 r10 = [in0], 8	;;
	ld8 r11 = [in0], 8	;;
	ld8 r12 = [in0], 8	;;	/* restore sp */
	ld8 r13 = [in0], 8	;;
	ld8 r14 = [in0], 8	;;
	ld8 r15 = [in0], 8	;;
	ld8 r16 = [in0], 8	;;
	ld8 r17 = [in0], 8	;;
	ld8 r18 = [in0], 8	;;
	ld8 r19 = [in0], 8	;;
	ld8 r20 = [in0], 8	;;
	ld8 r21 = [in0], 8	;;
	ld8 r22 = [in0], 8	;;
	ld8 r23 = [in0], 8	;;
	ld8 r24 = [in0], 8	;;
	ld8 r25 = [in0], 8	;;
	ld8 r26 = [in0], 8	;;
	ld8 r27 = [in0], 8	;;
	ld8 r28 = [in0], 8	;;
	ld8 r29 = [in0], 8	;;
	ld8 r30 = [in0], 8	;;
	ld8 r31 = [in0], 8	;;

	/* restore pc */
	ld8 loc1 = [in0], 8	;;
	mov b0 = loc1
	
	mov ar.pfs = loc0
	
	mov r8 = r0			/* context_restore returns 0 */
	br.ret.sptk.many b0
