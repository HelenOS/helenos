/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

.org 0

## User-space task entry point
#
# r6 contains the PCB pointer
#
SYMBOL(_start)
	#
	# Create the first stack frame.
	#
	li %r3, 0
	stw %r3, 0(%r1)
	stwu %r1, -16(%r1)

	# Pass the PCB pointer to __c_start() as the first argument.
	# The first argument is passed in r3.
	mr %r3, %r6
	bl __c_start
