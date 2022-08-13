/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

## User-space thread entry point for all but the first threads.
#
#
SYMBOL(__thread_entry)
	#
	# Create the first stack frame.
	#
	save %sp, -176, %sp
	flushw
	add %g0, -0x7ff, %fp

	#
	# Propagate the input arguments to the new window.
	#

	call __thread_main		! %o0 contains address of uarg
	mov %i0, %o0

	! not reached
