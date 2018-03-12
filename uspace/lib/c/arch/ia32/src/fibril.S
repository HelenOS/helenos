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
# Save CPU context to the context_t variable
# pointed by the 1st argument. Returns 0 in EAX.
#
FUNCTION_BEGIN(__setjmp)
	movl 0(%esp), %eax  # the caller's return %eip
	movl 4(%esp), %edx  # address of the context variable to save context to

	# save registers to the context structure
	movl %esp, CONTEXT_OFFSET_SP(%edx)	# %esp -> ctx->sp
	movl %eax, CONTEXT_OFFSET_PC(%edx)	# %eip -> ctx->pc
	movl %ebx, CONTEXT_OFFSET_EBX(%edx)	# %ebx -> ctx->ebx
	movl %esi, CONTEXT_OFFSET_ESI(%edx)	# %esi -> ctx->esi
	movl %edi, CONTEXT_OFFSET_EDI(%edx)	# %edi -> ctx->edi
	movl %ebp, CONTEXT_OFFSET_EBP(%edx)	# %ebp -> ctx->ebp

	# save TLS
	movl %gs:0, %eax
	movl %eax, CONTEXT_OFFSET_TLS(%edx)	# tls -> ctx->tls

	xorl %eax, %eax		# __setjmp returns 0
	ret
FUNCTION_END(__setjmp)

## Restore saved CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns second argument in EAX.
#
FUNCTION_BEGIN(__longjmp)
	movl 4(%esp), %eax  # address of the context variable to restore context from
	movl 8(%esp), %ecx  # return value

	# restore registers from the context structure
	movl CONTEXT_OFFSET_SP(%eax),%esp	# ctx->sp -> %esp
	movl CONTEXT_OFFSET_PC(%eax),%edx	# ctx->pc -> \pc
	movl CONTEXT_OFFSET_EBX(%eax),%ebx	# ctx->ebx -> %ebx
	movl CONTEXT_OFFSET_ESI(%eax),%esi	# ctx->esi -> %esi
	movl CONTEXT_OFFSET_EDI(%eax),%edi	# ctx->edi -> %edi
	movl CONTEXT_OFFSET_EBP(%eax),%ebp	# ctx->ebp -> %ebp

	movl %edx, 0(%esp)  # ctx->pc -> saver's return %eip

	# set thread local storage
	movl CONTEXT_OFFSET_TLS(%eax), %edx	# Set arg1 to TLS addr
	movl %edx, %gs:0

	movl %ecx, %eax
	ret
FUNCTION_END(__longjmp)

