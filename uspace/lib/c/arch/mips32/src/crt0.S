/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <libarch/stack.h>

.text

.set noreorder
.option pic2

## User-space task entry point
#
# $a0 ($4) contains the PCB pointer
#
FUNCTION_BEGIN(__start)
	.ent __start
	.frame $sp, ABI_STACK_FRAME, $ra
	.cpload $t9

	# Allocate the stack frame.
	addiu $sp, -ABI_STACK_FRAME

	# Allow PIC code
	.cprestore 16

	# Pass pcb_ptr to __c_start() as the first argument. It is already
	# in $a0. As the first argument is passed in $a0, no operation
	# is needed.

	jal __c_start
	nop

	#
	# Not reached.
	#
	addiu $sp, ABI_STACK_FRAME
	.end __start
FUNCTION_END(__start)
