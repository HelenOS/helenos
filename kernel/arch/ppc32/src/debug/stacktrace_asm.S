/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/asm/regname.h>

.text

FUNCTION_BEGIN(frame_pointer_get)
	mr r3, sp
	blr
FUNCTION_END(frame_pointer_get)

FUNCTION_BEGIN(program_counter_get)
	mflr r3
	blr
FUNCTION_END(program_counter_get)
