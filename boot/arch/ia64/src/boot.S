/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/arch.h>

.section BOOTSTRAP, "ax"

SYMBOL(start)
	movl gp = __gp

	#
	# Save the boot parameter structure address passed from the
	# ELILO-compatible EFI loader.
	#
	movl r8 = bootpar ;;
	st8 [r8] = r28

	#
	# Initialize the register stack to some sane value.
	#
	mov ar.rsc = r0
	movl r8 = initial_stack ;;
	mov ar.bspstore = r8
	loadrs

	#
	# Initialize memory stack to some sane value and allocate a scratch area
	# on it.
	#
	movl r12 = initial_stack_top ;;
	add r12 = -16, r12

	#
	# Disable Floating-point High register set (f32 - f127)
	#
	ssm (1 << 19) ;;
	srlz.i
	srlz.d ;;

	br.call.sptk.many rp = bootstrap

.bss

SYMBOL(bootpar)
	.quad 0

.align STACK_SIZE
initial_stack:
	.space STACK_SIZE
initial_stack_top:
