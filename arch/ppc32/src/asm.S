#
# Copyright (C) 2005 Martin Decky
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

#include <arch/asm/regname.h>

.text

.global cpu_halt
.global cpu_sleep
.global memsetb
.global memcpy

cpu_halt:
	b cpu_halt

cpu_sleep:
	b cpu_sleep

memsetb:
	rlwimi r5, r5, 8, 16, 23
	rlwimi r5, r5, 16, 0, 15
	
	addi r14, r3, -4
	
	cmplwi 0, r4, 4
	blt 7f
	
	stwu r5, 4(r14)
	beqlr
	
	andi. r15, r14, 3
	add r4, r15, r4
	subf r14, r15, r14
	srwi r15, r4, 2
	mtctr r15
	
	bdz 6f
	
	1:
		stwu r5, 4(r14)
		bdnz 1b
	
	6:
	
	andi. r4, r4, 3
	
	7:
	
	cmpwi 0, r4, 0
	beqlr
	
	mtctr r4
	addi r6, r6, 3
	
	8:
	
	stbu r5, 1(r14)
	bdnz 8b
	
	blr

memcpy:
	# TODO
	blr
