/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

.set noat
.set noreorder

FUNCTION_BEGIN(stacktrace_prepare)
FUNCTION_BEGIN(stacktrace_fp_get)
FUNCTION_BEGIN(stacktrace_pc_get)
	j $ra
	xor $v0, $v0
FUNCTION_END(stacktrace_prepare)
FUNCTION_END(stacktrace_fp_get)
FUNCTION_END(stacktrace_pc_get)
