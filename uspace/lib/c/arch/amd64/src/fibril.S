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

.text

.global context_save
.global context_restore

#include <libarch/fibril_context.h>

## Save current CPU context
#
# Save CPU context to context_t variable
# pointed by the 1st argument. Returns 1 in EAX.
#
context_save:
	movq (%rsp), %rdx     # the caller's return %eip
	
	# in %rdi is passed 1st argument
	movq %rdx, CONTEXT_OFFSET_PC(%rdi)
	movq %rsp, CONTEXT_OFFSET_SP(%rdi)
	
	movq %rbx, CONTEXT_OFFSET_RBX(%rdi)
	movq %rbp, CONTEXT_OFFSET_RBP(%rdi)
	movq %r12, CONTEXT_OFFSET_R12(%rdi)
	movq %r13, CONTEXT_OFFSET_R13(%rdi)
	movq %r14, CONTEXT_OFFSET_R14(%rdi)
	movq %r15, CONTEXT_OFFSET_R15(%rdi)
	
	# save TLS
	movq %fs:0, %rax
	movq %rax, CONTEXT_OFFSET_TLS(%rdi)
	
	xorl %eax, %eax                      # context_save returns 1
	incl %eax
	ret

## Restore current CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns 0 in EAX.
#
context_restore:
	
	movq CONTEXT_OFFSET_R15(%rdi), %r15
	movq CONTEXT_OFFSET_R14(%rdi), %r14
	movq CONTEXT_OFFSET_R13(%rdi), %r13
	movq CONTEXT_OFFSET_R12(%rdi), %r12
	movq CONTEXT_OFFSET_RBP(%rdi), %rbp
	movq CONTEXT_OFFSET_RBX(%rdi), %rbx
	
	movq CONTEXT_OFFSET_SP(%rdi), %rsp   # ctx->sp -> %rsp
	
	movq CONTEXT_OFFSET_PC(%rdi), %rdx
	
	movq %rdx,(%rsp)
	
	# Set thread local storage
	movq CONTEXT_OFFSET_TLS(%rdi), %rdi  # Set arg1 to TLS addr
	movl $1, %eax                        # SYS_TLS_SET
	syscall
	
	xorl %eax, %eax                      # context_restore returns 0
	ret

