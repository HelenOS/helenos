/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

INTEL_CPUID_STANDARD = 1
INTEL_SEP = 11

.text

.org 0

## User-space task entry point
#
# %edi contains the PCB pointer
#
SYMBOL(_start)
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

	# Pass the PCB pointer to __c_start as the first argument
	pushl %edi
	call __c_start
