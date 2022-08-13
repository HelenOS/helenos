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
	movl 4(%esp),%ecx	# move argument to %ecx
0:	lahf
	dec %ecx
	jnz 0b
	ret
FUNCTION_END(asm_delay_loop)

FUNCTION_BEGIN(asm_fake_loop)
	movl 4(%esp),%ecx	# move argument to %ecx
0:	lahf
	dec %ecx
	jz 0b
	ret
FUNCTION_END(asm_fake_loop)

