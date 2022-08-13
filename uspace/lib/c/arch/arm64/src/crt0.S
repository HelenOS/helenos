/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

.org 0

## User-space task entry point
#
# x0 contains the PCB pointer
#
SYMBOL(_start)
	#
	# Create the first stack frame.
	#
	mov x29, #0
	stp x29, x30, [sp, #-16]!
	mov x29, sp

	# Pass pcb_ptr to __c_start as the first argument (in x0)
	bl __c_start
