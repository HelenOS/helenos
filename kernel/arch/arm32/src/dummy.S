/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt, Pavel Jancik, Petr Stepan
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(calibrate_delay_loop)
	mov	pc, lr
FUNCTION_END(calibrate_delay_loop)

FUNCTION_BEGIN(asm_delay_loop)
	mov	pc, lr
FUNCTION_END(asm_delay_loop)
