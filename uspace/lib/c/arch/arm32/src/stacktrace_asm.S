/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(stacktrace_prepare)
	mov pc, lr
FUNCTION_END(stacktrace_prepare)

FUNCTION_BEGIN(stacktrace_fp_get)
	mov r0, fp
	mov pc, lr
FUNCTION_END(stacktrace_fp_get)

FUNCTION_BEGIN(stacktrace_pc_get)
	mov r0, lr
	mov pc, lr
FUNCTION_END(stacktrace_pc_get)
