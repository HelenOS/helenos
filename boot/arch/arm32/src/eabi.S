/*
 * SPDX-FileCopyrightText: 2012 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(__aeabi_idiv)
	push {lr}
	bl __divsi3
	pop {lr}
	mov pc, lr
FUNCTION_END(__aeabi_idiv)

FUNCTION_BEGIN(__aeabi_uidiv)
	push {lr}
	bl __udivsi3
	pop {lr}
	mov pc, lr
FUNCTION_END(__aeabi_uidiv)

FUNCTION_BEGIN(__aeabi_idivmod)
	push {lr}
	sub sp, sp, #12
	add r2, sp, #4
	bl __udivmodsi3
	ldr r1, [sp, #4]
	add sp, sp, #12
	pop {lr}
	mov pc, lr
FUNCTION_END(__aeabi_idivmod)

FUNCTION_BEGIN(__aeabi_uidivmod)
	push {lr}
	sub sp, sp, #12
	add r2, sp, #4
	bl __udivmodsi3
	ldr r1, [sp, #4]
	add sp, sp, #12
	pop {lr}
	mov pc, lr
FUNCTION_END(__aeabi_uidivmod)

FUNCTION_BEGIN(__aeabi_ldivmod)
	push {lr}
	sub sp, sp, #24
	push {sp}
	bl __divmoddi3
	add sp, sp, #4
	pop {r2, r3}
	add sp, sp, #16
	pop {lr}
	mov pc, lr
FUNCTION_END(__aeabi_ldivmod)

FUNCTION_BEGIN(__aeabi_uldivmod)
	push {lr}
	sub sp, sp, #24
	push {sp}
	bl __udivmoddi3
	add sp, sp, #4
	pop {r2, r3}
	add sp, sp, #16
	pop {lr}
	mov pc, lr
FUNCTION_END(__aeabi_uldivmod)

