/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(asm_delay_loop)
	blr
FUNCTION_END(asm_delay_loop)

FUNCTION_BEGIN(cpu_halt)
	b cpu_halt
FUNCTION_END(cpu_halt)
