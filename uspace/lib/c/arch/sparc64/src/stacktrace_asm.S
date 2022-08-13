/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <libarch/stack.h>

.text

FUNCTION_BEGIN(stacktrace_prepare)
	save %sp, -(STACK_WINDOW_SAVE_AREA_SIZE+STACK_ARG_SAVE_AREA_SIZE), %sp
	# Flush all other windows to memory so that we can read their contents.
	flushw
	ret
	restore
FUNCTION_END(stacktrace_prepare)

FUNCTION_BEGIN(stacktrace_fp_get)
	# Add the stack bias to %sp to get the actual address.
	retl
	add %sp, STACK_BIAS, %o0
FUNCTION_END(stacktrace_fp_get)

FUNCTION_BEGIN(stacktrace_pc_get)
	retl
	mov %o7, %o0
FUNCTION_END(stacktrace_pc_get)
