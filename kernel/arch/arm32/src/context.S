#
# Copyright (c) 2007 Petr Stepan
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

FUNCTION_BEGIN(context_save_arch)
	stmfd sp!, {r1}
	mrs r1, cpsr
	and r1, r1, #0x1f
	stmia r0!, {r1}
	ldmfd sp!, {r1}

	stmia r0!, {sp, lr}
	stmia r0!, {r4-r11}

	mov r0, #1
	mov pc, lr
FUNCTION_END(context_save_arch)

FUNCTION_BEGIN(context_restore_arch)
	ldmia r0!, {r4}
	mrs r5, cpsr
	bic r5, r5, #0x1f
	orr r5, r5, r4
	msr cpsr_c, r5

	ldmia r0!, {sp, lr}
	ldmia r0!, {r4-r11}

	mov r0, #0
	mov pc, lr
FUNCTION_END(context_restore_arch)
