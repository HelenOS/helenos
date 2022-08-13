/*
 * SPDX-FileCopyrightText: 2007 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.data

OBJECT_BEGIN(__syscall_fast_func)
	.long __syscall_slow
OBJECT_END(__syscall_fast_func)

.text

/** Syscall wrapper - INT $0x30 version.
 *
 * Mind the order of arguments. First two arguments and the syscall number go to
 * scratch registers. An optimized version of this wrapper for fewer arguments
 * could benefit from this and not save unused registers on the stack.
 */
FUNCTION_BEGIN(__syscall_slow)
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
FUNCTION_END(__syscall_slow)

/** Syscall wrapper - SYSENTER version.
 *
 * This is an optimized version of syscall for four or less arguments.  Note
 * that EBP and EDI are used to remember user stack address and the return
 * address. The kernel part doesn't save DS, ES and FS so the handler restores
 * these to the selector immediately following CS (it must be the flat data
 * segment, otherwise the SYSENTER wouldn't work in the first place).
 */
FUNCTION_BEGIN(__syscall_fast)
	pushl %ebx
	pushl %esi
	pushl %edi
	pushl %ebp
	mov %esp, %ebp
	lea ra, %edi
	movl 20(%esp), %edx	# First argument.
	movl 24(%esp), %ecx	# Second argument.
	movl 28(%esp), %ebx	# Third argument.
	movl 32(%esp), %esi	# Fourth argument.
	movl 44(%esp), %eax	# Syscall number.
	sysenter
ra:
	movw %cs, %cx
	addw $8, %cx
	movw %cx, %ds
	movw %cx, %es
	movw %cx, %fs
	popl %ebp
	popl %edi
	popl %esi
	popl %ebx
	ret
FUNCTION_END(__syscall_fast)
