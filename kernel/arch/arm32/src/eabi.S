#
# Copyright (c) 2012 Martin Decky
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
