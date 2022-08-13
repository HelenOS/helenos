/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(asm_delay_loop)
	ret
FUNCTION_END(asm_delay_loop)

FUNCTION_BEGIN(early_putuchar)
	ret
FUNCTION_END(early_putuchar)

FUNCTION_BEGIN(cpu_halt)
	j cpu_halt
FUNCTION_END(cpu_halt)
