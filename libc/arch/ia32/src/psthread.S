#
# Copyright (C) 2001-2004 Jakub Jermar
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


## Save current CPU context
#
# Save CPU context to the context_t variable
# pointed by the 1st argument. Returns 1 in EAX.
#
context_save:
	movl 0(%esp),%eax	# the caller's return %eip
	movl 4(%esp),%edx	# address of the kernel_context variable to save context to

	movl %esp,0(%edx)	# %esp -> ctx->sp
	movl %eax,4(%edx)	# %eip -> ctx->pc
	movl %ebx,8(%edx)	# %ebx -> ctx->ebx
	movl %esi,12(%edx)	# %esi -> ctx->esi
	movl %edi,16(%edx)	# %edi -> ctx->edi
	movl %ebp,20(%edx)	# %ebp -> ctx->ebp

	# Save TLS
	movl %gs:0, %eax
	movl %eax, 24(%edx)     # tls -> ctx->tls
	
	xorl %eax,%eax		# context_save returns 1
	incl %eax
	ret


## Restore saved CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns 0 in EAX.
#
context_restore:
	movl 4(%esp),%eax	# address of the kernel_context variable to restore context from
	movl 0(%eax),%esp	# ctx->sp -> %esp
	movl 4(%eax),%edx	# ctx->pc -> %edx
	movl 8(%eax),%ebx	# ctx->ebx -> %ebx
	movl 12(%eax),%esi	# ctx->esi -> %esi
	movl 16(%eax),%edi	# ctx->edi -> %edi
	movl 20(%eax),%ebp	# ctx->ebp -> %ebp

	movl %edx,0(%esp)	# ctx->pc -> saver's return %eip
	
	# Set thread local storage
	movl 24(%eax), %eax   # Set arg1 to TLS addr
	movl $1, %edi         # Syscall 1
	int $0x30
	
	xorl %eax,%eax		# context_restore returns 0
	ret
