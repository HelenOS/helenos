/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

.org 0

## User-space task entry point
#
# %rdi contains the PCB pointer
#
SYMBOL(_start)
	#
	# Create the first stack frame.
	#
	pushq $0
	pushq $0
	movq %rsp, %rbp

	# %rdi was deliberately chosen as the first argument is also in %rdi
	# Pass PCB pointer to __c_start (no operation)
	call FUNCTION_REF(__c_start)
