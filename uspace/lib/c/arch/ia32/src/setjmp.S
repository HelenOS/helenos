#
# Copyright (c) 2008 Josef Cejka
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

#include <libarch/context_offset.h>

.text
.global setjmp
.global longjmp

.type setjmp,@function
setjmp:
	movl 0(%esp),%eax	# save pc value into eax	
	movl 4(%esp),%edx	# address of the jmp_buf structure to save context to 

		# save registers to the jmp_buf structure
	CONTEXT_SAVE_ARCH_CORE %edx %eax

	xorl %eax,%eax		# set_jmp returns 0
	ret

.type longjmp,@function
longjmp:

	movl 4(%esp), %ecx	# put address of jmp_buf into ecx
	movl 8(%esp), %eax	# put return value into eax	

		# restore registers from the jmp_buf structure
	CONTEXT_RESTORE_ARCH_CORE %ecx %edx

	movl %edx,0(%esp)	# put saved pc on stack
	ret

