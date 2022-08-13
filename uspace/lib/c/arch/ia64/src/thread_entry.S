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
SYMBOL_BEGIN(__thread_entry)
	alloc loc0 = ar.pfs, 0, 1, 1, 0

#ifndef CONFIG_RTLD
	# XXX This does not work in a shared library
	movl gp = __gp
#endif

	#
	# r8 contains address of uarg structure.
	#

	mov out0 = r8 ;;
	# XXX br.call.sptk.many b0 = FUNCTION_REF(__thread_main)
	br.call.sptk.many b0 = __thread_main

	#
	# Not reached.
	#

SYMBOL_END(__thread_entry)
