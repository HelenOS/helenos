/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(stacktrace_prepare)
	br.ret.sptk.many b0
FUNCTION_END(stacktrace_prepare)

FUNCTION_BEGIN(stacktrace_fp_get)
FUNCTION_BEGIN(stacktrace_pc_get)
	mov r8 = r0
	br.ret.sptk.many b0
FUNCTION_END(stacktrace_fp_get)
FUNCTION_END(stacktrace_pc_get)
