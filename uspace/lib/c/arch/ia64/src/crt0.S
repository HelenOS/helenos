/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

.org 0
.align 16

## User-space task entry point
#
# r2 contains the PCB pointer
#
SYMBOL(_start)
	alloc loc0 = ar.pfs, 0, 1, 2, 0
	movl gp = __gp

	# Pass PCB pointer as the first argument to __c_start
	mov out0 = r2 ;;
	br.call.sptk.many b0 = __c_start

