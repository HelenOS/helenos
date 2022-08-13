/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

## void entry_point_jmp(void *entry_point, void *pcb);
#
# %o0	contains entry_point
# %o1	contains pcb
#
# Jump to program entry point
SYMBOL(entry_point_jmp)
	# Pass pcb pointer to entry point in %o1. As it is already
	# there, no action is needed.
	call %o0
	nop
	# fixme: use branch instead of call
