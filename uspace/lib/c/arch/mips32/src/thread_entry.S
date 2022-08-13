/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <libarch/stack.h>

.text

.set noat
.set noreorder
.option pic2

## User-space thread entry point for all but the first threads.
#
#
SYMBOL(__thread_entry)
	.ent __thread_entry
	.frame $sp, ABI_STACK_FRAME, $ra
	.cpload $t9

	#
	# v0 contains address of uarg.
	#
	add $a0, $v0, 0

	# Allocate the stack frame.
	addiu $sp, -ABI_STACK_FRAME

	# Allow PIC code
	.cprestore 16

	jal __thread_main
	nop

	#
	# Not reached.
	#
	addiu $sp, ABI_STACK_FRAME
	.end __thread_entry
