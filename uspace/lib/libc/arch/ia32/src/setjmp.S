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

.text
.global setjmp
.global longjmp

.type setjmp,@function
setjmp:
	movl	0(%esp), %ecx	# save current pc
	leal	4(%esp), %edx	# save stack pointer
	movl	(%edx), %eax	# get jmp_buf pointer

		# Save registers 
	movl %ebx, 0(%eax)
	movl %esi, 4(%eax)
	movl %edi, 8(%eax)
	movl %ebp, 12(%eax)
	movl %edx, 16(%eax)

	movl %ecx, 20(%eax) # save pc
	ret

.type longjmp,@function
longjmp:

	movl 4(%esp), %ecx	# put address of jmp_buf into ecx
	movl 8(%esp), %eax	# put return value into eax	

		# restore all registers
	movl 0(%ecx), %ebx
	movl 4(%ecx), %esi
	movl 8(%ecx), %edi
	movl 12(%ecx), %ebp
	movl 16(%ecx), %esp
	
	movl 20(%ecx), %edx # saved return address 
	jmp *%edx
