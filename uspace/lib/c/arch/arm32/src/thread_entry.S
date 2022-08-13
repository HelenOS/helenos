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
	mov fp, #0
	mov ip, sp
	push {fp, ip, lr, pc}
	sub fp, ip, #4

	b __thread_main
