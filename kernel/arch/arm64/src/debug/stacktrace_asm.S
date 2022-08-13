/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(frame_pointer_get)
	mov x0, x29
	ret
FUNCTION_END(frame_pointer_get)

FUNCTION_BEGIN(program_counter_get)
	mov x0, x30
	ret
FUNCTION_END(program_counter_get)
