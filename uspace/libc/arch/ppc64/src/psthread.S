#
# Copyright (c) 2006 Martin Decky
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

#include <libarch/regname.h>
#include <libarch/context_offset.h>

.macro CONTEXT_STORE r
	stw sp, OFFSET_SP(\r)
	stw r2, OFFSET_R2(\r)
	stw r13, OFFSET_R13(\r)
	stw r14, OFFSET_R14(\r)
	stw r15, OFFSET_R15(\r)
	stw r16, OFFSET_R16(\r)
	stw r17, OFFSET_R17(\r)
	stw r18, OFFSET_R18(\r)
	stw r19, OFFSET_R19(\r)
	stw r20, OFFSET_R20(\r)
	stw r21, OFFSET_R21(\r)
	stw r22, OFFSET_R22(\r)
	stw r23, OFFSET_R23(\r)
	stw r24, OFFSET_R24(\r)
	stw r25, OFFSET_R25(\r)
	stw r26, OFFSET_R26(\r)
	stw r27, OFFSET_R27(\r)
	stw r28, OFFSET_R28(\r)
	stw r29, OFFSET_R29(\r)
	stw r30, OFFSET_R30(\r)
	stw r31, OFFSET_R31(\r)
.endm

.macro CONTEXT_LOAD r
	lwz sp, OFFSET_SP(\r)
	lwz r2, OFFSET_R2(\r)
	lwz r13, OFFSET_R13(\r)
	lwz r14, OFFSET_R14(\r)
	lwz r15, OFFSET_R15(\r)
	lwz r16, OFFSET_R16(\r)
	lwz r17, OFFSET_R17(\r)
	lwz r18, OFFSET_R18(\r)
	lwz r19, OFFSET_R19(\r)
	lwz r20, OFFSET_R20(\r)
	lwz r21, OFFSET_R21(\r)
	lwz r22, OFFSET_R22(\r)
	lwz r23, OFFSET_R23(\r)
	lwz r24, OFFSET_R24(\r)
	lwz r25, OFFSET_R25(\r)
	lwz r26, OFFSET_R26(\r)
	lwz r27, OFFSET_R27(\r)
	lwz r28, OFFSET_R28(\r)
	lwz r29, OFFSET_R29(\r)
	lwz r30, OFFSET_R30(\r)
	lwz r31, OFFSET_R31(\r)
.endm

context_save:
	CONTEXT_STORE r3
	
	mflr r4
	stw r4, OFFSET_PC(r3)
	
	mfcr r4
	stw r4, OFFSET_CR(r3)
	
	# context_save returns 1
	li r3, 1
	blr


context_restore:
	CONTEXT_LOAD r3
	
	lwz r4, OFFSET_CR(r3)
	mtcr r4
	
	lwz r4, OFFSET_PC(r3)
	mtlr r4
	
	# context_restore returns 0
	li r3, 0
	blr
