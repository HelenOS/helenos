/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

.set noat
.set noreorder

FUNCTION_BEGIN(frame_pointer_get)
	j $ra
	move $v0, $sp
FUNCTION_END(frame_pointer_get)

FUNCTION_BEGIN(program_counter_get)
	j $ra
	move $v0, $ra
FUNCTION_END(program_counter_get)
