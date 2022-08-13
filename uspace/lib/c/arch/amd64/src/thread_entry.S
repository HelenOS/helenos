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
	#
	# Create the first stack frame.
	#
	pushq $0
	pushq $0
	movq %rsp, %rbp

	#
	# RAX contains address of uarg
	#
	movq %rax, %rdi
	call FUNCTION_REF(__thread_main)
SYMBOL_END(__thread_entry)
