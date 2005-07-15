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

# The code below just interfaces the CPUID instruction.
# CPU recognition logic is contained in higher-level functions.

.text

.global has_cpuid
.global cpuid
.global rdtsc


## Determine CPUID support
#
# Return 0 in EAX if CPUID is not support, 1 if supported.
#
has_cpuid:
	push %ebx
	
	pushf			# store flags
	popl %eax		# read flags
	movl %eax,%ebx		# copy flags
	btcl $21,%ebx		# swap the ID bit
	pushl %ebx
	popf			# propagate the change into flags
	pushf
	popl %ebx		# read flags	
	andl $(1<<21),%eax	# interested only in ID bit
	andl $(1<<21),%ebx
	xorl %ebx,%eax		# 0 if not supported, 1 if supported
	
	pop %ebx
	ret


## Get CPUID data
#
# This code is just an interfaces the CPUID instruction, CPU recognition
# logic is contained in higher-level functions.
#
# The C prototype is:
#   void cpuid(__u32 cmd, struct cpu_info *info)
#
# @param cmd  CPUID command.
# @param info Buffer to store CPUID output.
#
cpuid:
	pushl %ebp
	movl %esp,%ebp
	pusha

	movl 8(%ebp),%eax	# load the command into %eax
	movl 12(%ebp),%esi	# laod the address of the info struct

	cpuid	
	movl %eax,0(%esi)
	movl %ebx,4(%esi)
	movl %ecx,8(%esi)
	movl %edx,12(%esi)

	popa
	popl %ebp
	ret

rdtsc:
	rdtsc
	ret
