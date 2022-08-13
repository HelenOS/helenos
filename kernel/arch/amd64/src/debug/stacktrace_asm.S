/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(frame_pointer_get)
	movq %rbp, %rax
	ret
FUNCTION_END(frame_pointer_get)

FUNCTION_BEGIN(program_counter_get)
	movq (%rsp), %rax
	ret
FUNCTION_END(program_counter_get)
