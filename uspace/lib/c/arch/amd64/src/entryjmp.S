/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

## void entry_point_jmp(void *entry_point, void *pcb);
#
# %rdi	contains entry_point
# %rsi	contains pcb
#
# Jump to program entry point
SYMBOL(entry_point_jmp)
	# use standard amd32 prologue not to confuse anybody
	push %rbp
	movq %rsp, %rbp

	# pcb must be passed in %rdi, use %rdx as a scratch register
	mov %rdi, %rdx
	mov %rsi, %rdi

	# jump to entry point
	jmp *%rdx
