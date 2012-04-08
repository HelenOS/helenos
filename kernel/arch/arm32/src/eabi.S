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

.text

.global __aeabi_idiv
.global __aeabi_uidiv

.global __aeabi_idivmod
.global __aeabi_uidivmod

.global __aeabi_ldivmod
.global __aeabi_uldivmod

__aeabi_idiv:
	push {sp, lr}
	bl __divsi3
	ldr lr, [sp, #4]
	add sp, sp, #8
	bx lr

__aeabi_uidiv:
	push {sp, lr}
	bl __udivsi3
	ldr lr, [sp, #4]
	add sp, sp, #8
	bx lr

__aeabi_idivmod:
	sub sp, sp, #8
	push {sp, lr}
	bl __divmodsi3
	ldr lr, [sp, #4]
	add sp, sp, #8
	pop {r1, r2}
	bx lr

__aeabi_uidivmod:
	sub sp, sp, #8
	push {sp, lr}
	bl __udivmodsi3
	ldr lr, [sp, #4]
	add sp, sp, #8
	pop {r1, r2}
	bx lr

__aeabi_ldivmod:
	sub sp, sp, #8
	push {sp, lr}
	bl __divmoddi3
	ldr lr, [sp, #4]
	add sp, sp, #8
	pop {r2, r3}
	bx lr

__aeabi_uldivmod:
	sub sp, sp, #8
	push {sp, lr}
	bl __udivmoddi3
	ldr lr, [sp, #4]
	add sp, sp, #8
	pop {r2, r3}
	bx lr
