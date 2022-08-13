/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(calibrate_delay_loop)
FUNCTION_BEGIN(asm_delay_loop)
FUNCTION_BEGIN(cpu_sleep)
	br.ret.sptk.many b0
FUNCTION_END(calibrate_delay_loop)
FUNCTION_END(asm_delay_loop)
FUNCTION_END(cpu_sleep)

