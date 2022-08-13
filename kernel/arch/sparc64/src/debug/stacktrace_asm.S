/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/stack.h>

.text

FUNCTION_BEGIN(frame_pointer_get)
	# Add the stack bias to %sp to get the actual address.
	retl
	add %sp, STACK_BIAS, %o0
FUNCTION_END(frame_pointer_get)

FUNCTION_BEGIN(program_counter_get)
	retl
	mov %o7, %o0
FUNCTION_END(program_counter_get)

FUNCTION_BEGIN(alloc_window_and_flush)
	save %sp, -(STACK_WINDOW_SAVE_AREA_SIZE+STACK_ARG_SAVE_AREA_SIZE), %sp
	# Flush all other windows to memory so that we can read their contents.
	flushw
	ret
	restore
FUNCTION_END(alloc_window_and_flush)

