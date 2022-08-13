/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

## void entry_point_jmp(void *entry_point, void *pcb);
#
# r0	contains entry_point
# r1	contains pcb
#
# Jump to program entry point
SYMBOL(entry_point_jmp)
	# load ras_page address to r2
	ldr r2, =ras_page
	ldr r2, [r2]
	# pcb is passed to the entry point in r1 (where it already is)
	mov r15, r0
