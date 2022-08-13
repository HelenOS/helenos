/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

.text

#include <abi/asmtool.h>
#include <libarch/regname.h>

FUNCTION_BEGIN(stacktrace_prepare)
	blr
FUNCTION_END(stacktrace_prepare)

FUNCTION_BEGIN(stacktrace_fp_get)
	mr r3, sp
	blr
FUNCTION_END(stacktrace_fp_get)

FUNCTION_BEGIN(stacktrace_pc_get)
	mflr r3
	blr
FUNCTION_END(stacktrace_pc_get)
