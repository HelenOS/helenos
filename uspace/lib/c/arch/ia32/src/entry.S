#
# Copyright (c) 2005 Martin Decky
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

INTEL_CPUID_STANDARD = 1
INTEL_SEP = 11

.section .init, "ax"

.org 0

## User-space task entry point
#
# %edi contains the PCB pointer
#
SYMBOL(__entry)
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	# Do not set %gs, it contains descriptor that can see TLS

#ifndef PROCESSOR_i486
	# Detect the mechanism used for making syscalls
	movl $(INTEL_CPUID_STANDARD), %eax
	cpuid
	bt $(INTEL_SEP), %edx
	jnc 0f
		leal __syscall_fast_func, %eax
		movl $__syscall_fast, (%eax)
	0:
#endif

	#
	# Create the first stack frame.
	#
	pushl $0
	pushl $0
	movl %esp, %ebp

	# Pass the PCB pointer to __main as the first argument
	pushl %edi
	call __main
