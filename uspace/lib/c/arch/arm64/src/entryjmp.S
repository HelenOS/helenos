/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

## void entry_point_jmp(void *entry_point, void *pcb);
#
# x0	contains entry_point
# x1	contains pcb
#
# Jump to program entry point
SYMBOL(entry_point_jmp)
	# Pass PCB in x0 to the entry point and jump to it.
	mov x2, x0
	mov x0, x1
	br x2
