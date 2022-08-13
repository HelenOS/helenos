/*
 * SPDX-FileCopyrightText: 2017 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/boot/boot.h>
#include <arch/cpu.h>

.section K_TEXT_START, "ax"

SYMBOL(kernel_image_start)
	/* Setup temporary stack */
	la sp, temp_stack

	/* Create the first stack frame */
	addi sp, sp, -16

	/* Call riscv64_pre_main() */
	jal riscv64_pre_main

	/* Call main_bsp() */
	jal main_bsp

	/* Not reached */
	csrci sstatus, SSTATUS_SIE_MASK
	hlt0:
		wfi
		j hlt0

.data

.align 16
.space TEMP_STACK_SIZE
temp_stack:
