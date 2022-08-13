/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

## User-space thread entry point for all but the first thread.
#
#
SYMBOL(__thread_entry)
	#
	# Create the first stack frame.
	#
	mov x29, #0
	stp x29, x30, [sp, #-16]!
	mov x29, sp

	b __thread_main
