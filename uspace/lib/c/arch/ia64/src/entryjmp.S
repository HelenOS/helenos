/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text
.explicit

## void entry_point_jmp(void *entry_point, void *pcb);
#
# in0 (r32)	contains entry_point
# in1 (r33)	contains pcb
#
# Jump to program entry point
SYMBOL(entry_point_jmp)
	# Pass pcb to the entry point in r2

	mov b6 = r32
	mov r2 = r33 ;;
	br b6 ;;
