/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(cpu_sleep)
	retl
	nop
FUNCTION_END(cpu_sleep)

FUNCTION_BEGIN(cpu_halt)
	ba %xcc, cpu_halt
	nop
FUNCTION_END(cpu_halt)

