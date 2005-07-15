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
# Save CPU context to the kernel_context variable
# pointed by the 1st argument. Returns 1 in EAX.
#
context_save:
	push %ebx

	movl 4(%esp),%eax	# the caller's return %eip
	movl 8(%esp),%ebx	# address of the kernel_context variable to save context to
	movl %eax,4(%ebx)	# %eip -> ctx->pc
	movl %esp,(%ebx)	# %esp -> ctx->sp

	movl %ebx,%eax
	pop %ebx

	movl %ebx,8(%eax)
	movl %ecx,12(%eax)
	movl %edx,16(%eax)
	movl %esi,20(%eax)
	movl %edi,24(%eax)
	movl %ebp,28(%eax)

	xorl %eax,%eax		# context_save returns 1
	incl %eax
	ret


## Restore current CPU context
#
# Restore CPU context from the kernel_context variable
# pointed by the 1st argument. Returns 0 in EAX.
#
context_restore:
	movl 4(%esp),%eax	# address of the kernel_context variable to restore context from
	movl (%eax),%esp	# ctx->sp -> %esp
	addl $4,%esp		# this is for the pop we don't do

	movl 8(%eax),%ebx
	movl 12(%eax),%ecx
	movl 16(%eax),%edx
	movl 20(%eax),%esi
	movl 24(%eax),%edi
	movl 28(%eax),%ebp

	movl 4(%eax),%eax
	movl %eax,(%esp)	# ctx->pc -> saver's return %eip
	xorl %eax,%eax		# context_restore returns 0
	ret
