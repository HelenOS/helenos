/*
 * Copyright (c) 2007 Michal Kebrt
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

.text

FUNCTION_BEGIN(memcpy_from_uspace)
FUNCTION_BEGIN(memcpy_to_uspace)
	add r3, r1, #3
	bic r3, r3, #3
	cmp r1, r3
	stmdb sp!, {r4, r5, lr}
	mov r5, r0 /* save dst */
	beq 4f

	1:
		cmp r2, #0
		movne ip, #0
		beq 3f

	2:
		ldrb r3, [ip, r1]
		strb r3, [ip, r0]
		add ip, ip, #1
		cmp ip, r2
		bne 2b

	3:
		mov r0, r5
		ldmia sp!, {r4, r5, pc}

	4:
		add r3, r0, #3
		bic r3, r3, #3
		cmp r0, r3
		bne 1b
		movs r4, r2, lsr #2
		moveq lr, r4
		beq 6f
		mov lr, #0
		mov ip, lr

	5:
		ldr r3, [ip, r1]
		add lr, lr, #1
		cmp lr, r4
		str r3, [ip, r0]
		add ip, ip, #4
		bne 5b

	6:
		ands r4, r2, #3
		beq 3b
		mov r3, lr, lsl #2
		add r0, r3, r0
		add ip, r3, r1
		mov r2, #0

	7:
		ldrb r3, [r2, ip]
		strb r3, [r2, r0]
		add r2, r2, #1
		cmp r2, r4
		bne 7b
		b 3b
FUNCTION_END(memcpy_from_uspace)
FUNCTION_END(memcpy_to_uspace)

SYMBOL(memcpy_from_uspace_failover_address)
SYMBOL(memcpy_to_uspace_failover_address)
	mov r0, #0
	ldmia sp!, {r4, r5, pc}

FUNCTION_BEGIN(early_putuchar)
	mov pc, lr
FUNCTION_END(early_putuchar)
