/*
 * SPDX-FileCopyrightText: 2012 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(__aeabi_read_tp)
	mov r0, r9
	mov pc, lr
FUNCTION_END(__aeabi_read_tp)
