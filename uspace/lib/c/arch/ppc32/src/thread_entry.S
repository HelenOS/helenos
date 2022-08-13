/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
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
	li %r4, 0
	stw %r4, 0(%r1)
	stwu %r1, -16(%r1)

	b __thread_main

