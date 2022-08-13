/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/arch.h>

.text

FUNCTION_BEGIN(halt)
	br halt
FUNCTION_END(halt)

FUNCTION_BEGIN(jump_to_kernel)
	alloc loc0 = ar.pfs, 2, 1, 0, 0
	mov r2 = in0 ;;			# Pass bootinfo address
	mov b1 = in1 ;;                 # Entry point address
	br.call.sptk.many b0 = b1;;
FUNCTION_END(jump_to_kernel)

