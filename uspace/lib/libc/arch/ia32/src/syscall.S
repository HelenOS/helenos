#
# Copyright (c) 2007 Jakub Jermar
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

/** Syscall wrapper.
 *
 * Mind the order of arguments. First two arguments and the syscall number go to
 * scratch registers. An optimized version of this wrapper for fewer arguments
 * could benefit from this and not save unused registers on the stack.
 */
.global __syscall
__syscall:
	pushl %ebx
	pushl %esi
	pushl %edi
	pushl %ebp
	movl 20(%esp), %edx	# First argument.
	movl 24(%esp), %ecx	# Second argument.
	movl 28(%esp), %ebx	# Third argument.
	movl 32(%esp), %esi	# Fourth argument.
	movl 36(%esp), %edi	# Fifth argument.
	movl 40(%esp), %ebp	# Sixth argument.
	movl 44(%esp), %eax	# Syscall number.
	int $0x30
	popl %ebp
	popl %edi
	popl %esi
	popl %ebx
	ret
