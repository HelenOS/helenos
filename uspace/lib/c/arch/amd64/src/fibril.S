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

#include <libarch/context_offset.h>

## Save current CPU context
#
# Save CPU context to context_t variable
# pointed by the 1st argument. Returns 1 in EAX.
#
context_save:
	movq (%rsp), %rdx     # the caller's return %eip
	
	# In %edi is passed 1st argument
	CONTEXT_SAVE_ARCH_CORE %rdi %rdx 
	
	# Save TLS
	movq %fs:0, %rax
	movq %rax, OFFSET_TLS(%rdi)
		
	xorl %eax, %eax		# context_save returns 1
	incl %eax
	ret


## Restore current CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns 0 in EAX.
#
context_restore:
	
	CONTEXT_RESTORE_ARCH_CORE %rdi %rdx
	
	movq %rdx,(%rsp)

	# Set thread local storage
	movq OFFSET_TLS(%rdi), %rdi   # Set arg1 to TLS addr
	movl $1, %eax		# SYS_TLS_SET
	syscall

	xorl %eax, %eax		# context_restore returns 0
	ret
