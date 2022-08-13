/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

## User-space thread entry point for all but the first threads.
#
#
SYMBOL_BEGIN(__thread_entry)
	mov %ss, %dx
	mov %dx, %ds
	mov %dx, %es
	mov %dx, %fs
	# Do not set %gs, it contains descriptor that can see TLS

	#
	# Create the first stack frame.
	#
	pushl $0
	pushl $0
	mov %esp, %ebp

	#
	# EAX contains address of uarg.
	#
	pushl %eax
	call __thread_main

	#
	# Not reached.
	#
SYMBOL_END(__thread_entry)
