/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

## void entry_point_jmp(void *entry_point, void *pcb);
#
# %r3	contains entry_point
# %r4	contains pcb
#
# Jump to program entry point
FUNCTION_BEGIN(entry_point_jmp)
	mtctr %r3
	mr %r6, %r4	# Pass pcb to the entry point in %r6
	bctr
FUNCTION_END(entry_point_jmp)
