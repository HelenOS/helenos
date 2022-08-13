/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#
# Micro second delay loop functions.
#

#include <abi/asmtool.h>

.text

FUNCTION_BEGIN(asm_delay_loop)
	0:
		dec %rdi
		jnz 0b

	ret
FUNCTION_END(asm_delay_loop)

FUNCTION_BEGIN(asm_fake_loop)
	0:
		dec %rdi
		jz 0b

	ret
FUNCTION_END(asm_fake_loop)

