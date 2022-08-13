/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/asm/regname.h>
#include <config.h>

.section K_TEXT_START, "ax"

SYMBOL(kernel_image_start)
	# load temporary kernel stack

	lis sp, kernel_stack@ha
	addi sp, sp, kernel_stack@l

	# set kernel stack for interrupt handling

	mr r31, sp
	subis r31, r31, 0x8000
	mtsprg0 r31

	# r3 contains physical address of bootinfo_t

	addis r3, r3, 0x8000
	bl ppc32_pre_main
	b main_bsp

.section K_DATA_START, "aw", @progbits

.align 12
kernel_stack_bottom:
	.space STACK_SIZE
kernel_stack:
