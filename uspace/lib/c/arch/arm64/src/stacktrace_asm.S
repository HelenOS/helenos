/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(stacktrace_prepare)
	ret
FUNCTION_END(stacktrace_prepare)

FUNCTION_BEGIN(stacktrace_fp_get)
	mov x0, x29
	ret
FUNCTION_END(stacktrace_fp_get)

FUNCTION_BEGIN(stacktrace_pc_get)
	mov x0, x30
	ret
FUNCTION_END(stacktrace_pc_get)
