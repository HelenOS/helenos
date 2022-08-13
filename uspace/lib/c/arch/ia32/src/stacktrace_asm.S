/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(stacktrace_prepare)
	ret
FUNCTION_END(stacktrace_prepare)

FUNCTION_BEGIN(stacktrace_fp_get)
	movl %ebp, %eax
	ret
FUNCTION_END(stacktrace_fp_get)

FUNCTION_BEGIN(stacktrace_pc_get)
	movl (%esp), %eax
	ret
FUNCTION_END(stacktrace_pc_get)
