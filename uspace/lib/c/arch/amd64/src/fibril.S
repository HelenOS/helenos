#
# Copyright (c) 2001-2004 Jakub Jermar
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
#include <libarch/fibril_context.h>

.text

## Save current CPU context
#
# Save CPU context to context_t variable
# pointed by the 1st argument. Returns 0 in RAX.
#
FUNCTION_BEGIN(__context_save)
	movq (%rsp), %rdx     # the caller's return %eip

	# in %rdi is passed 1st argument
	movq %rdx, __CONTEXT_OFFSET_PC(%rdi)
	movq %rsp, __CONTEXT_OFFSET_SP(%rdi)

	movq %rbx, __CONTEXT_OFFSET_RBX(%rdi)
	movq %rbp, __CONTEXT_OFFSET_RBP(%rdi)
	movq %r12, __CONTEXT_OFFSET_R12(%rdi)
	movq %r13, __CONTEXT_OFFSET_R13(%rdi)
	movq %r14, __CONTEXT_OFFSET_R14(%rdi)
	movq %r15, __CONTEXT_OFFSET_R15(%rdi)

	movq %fs:0, %rax
	movq %rax, __CONTEXT_OFFSET_TLS(%rdi)

	xorq %rax, %rax                      # __context_save returns 0
	ret
FUNCTION_END(__context_save)

## Restore current CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns second argument in RAX.
#
FUNCTION_BEGIN(__context_restore)
	movq __CONTEXT_OFFSET_R15(%rdi), %r15
	movq __CONTEXT_OFFSET_R14(%rdi), %r14
	movq __CONTEXT_OFFSET_R13(%rdi), %r13
	movq __CONTEXT_OFFSET_R12(%rdi), %r12
	movq __CONTEXT_OFFSET_RBP(%rdi), %rbp
	movq __CONTEXT_OFFSET_RBX(%rdi), %rbx

	movq __CONTEXT_OFFSET_SP(%rdi), %rsp   # ctx->sp -> %rsp

	movq __CONTEXT_OFFSET_PC(%rdi), %rdx

	movq %rdx,(%rsp)

	movq __CONTEXT_OFFSET_TLS(%rdi), %rdi
	movq %rdi, %fs:0

	movq %rsi, %rax                      # __context_restore returns second argument
	ret
FUNCTION_END(__context_restore)

