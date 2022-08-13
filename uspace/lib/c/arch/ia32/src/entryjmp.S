/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

## void entry_point_jmp(void *entry_point, void *pcb);
#
# Jump to program entry point
SYMBOL(entry_point_jmp)
	# Use standard ia32 prologue not to confuse anybody
	push %ebp
	movl %esp, %ebp

	# %eax := entry_point
	movl 0x8(%ebp), %eax

	# %edi := pcb
	# pcb is passed to the entry point in %edi
	mov 0xc(%ebp), %edi

	# Save a tiny bit of stack space
	pop %ebp

	jmp *%eax
