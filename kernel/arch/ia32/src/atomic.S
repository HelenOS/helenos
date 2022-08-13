/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

#ifdef CONFIG_SMP

#
# This is a bus-and-hyperthreading-friendly implementation of spinlock
#
FUNCTION_BEGIN(spinlock_arch)
	pushl %eax
	pushl %ebx

	movl 12(%esp),%ebx

0:
	pause			# Pentium 4's with HT love this instruction
	movl (%ebx),%eax
	testl %eax,%eax
	jnz 0b			# lightweight looping while it is locked
	incl %eax
	xchgl %eax,(%ebx)	# now use the atomic operation
	testl %eax,%eax
	jnz 0b

	popl %ebx
	popl %eax
	ret
FUNCTION_END(spinlock_arch)

#endif
