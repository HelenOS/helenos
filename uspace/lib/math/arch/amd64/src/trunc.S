#
# Copyright (c) 2014 Martin Decky
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
#include <libarch/x87.h>

.text

FUNCTION_BEGIN(trunc_f64)
	pushq %rbp
	movq %rsp, %rbp

	# store x87 control word in the red zone

	fnstcw -8(%rbp)
	movw -8(%rbp), %ax

	# set rounding control to truncate
	# (no masking necessary for this flag)

	orw $X87_CONTROL_WORD_RC_TRUNCATE, %ax
	movw %ax, -16(%rbp)
	fldcw -16(%rbp)

	# truncate

	movsd %xmm0, -16(%rbp)
	fldl -16(%rbp)
	frndint
	fstpl -16(%rbp)
	movsd -16(%rbp), %xmm0

	# restore original control word

	fldcw -8(%rbp)

	leave
	retq
FUNCTION_END(trunc_f64)
