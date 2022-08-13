/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(frame_pointer_get)
	movl %ebp, %eax
	ret
FUNCTION_END(frame_pointer_get)

FUNCTION_BEGIN(program_counter_get)
	movl (%esp), %eax
	ret
FUNCTION_END(program_counter_get)
