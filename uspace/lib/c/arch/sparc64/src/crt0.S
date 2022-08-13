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
# %o0 contains uarg
# %o1 contains pcb_ptr
#
SYMBOL(_start)
	#
	# Create the first stack frame.
	#
	save %sp, -176, %sp
	flushw
	add %g0, -0x7ff, %fp

	# Pass pcb_ptr as the first argument to __c_start()
	call __c_start
	mov %i1, %o0
