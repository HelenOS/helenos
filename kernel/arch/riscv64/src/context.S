/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/context_struct.h>

.text

FUNCTION_BEGIN(context_save_arch)
	sd gp, CONTEXT_OFFSET_GP(a0)
	sd tp, CONTEXT_OFFSET_TP(a0)
	sd s0, CONTEXT_OFFSET_S0(a0)
	sd s1, CONTEXT_OFFSET_S1(a0)
	sd s2, CONTEXT_OFFSET_S2(a0)
	sd s3, CONTEXT_OFFSET_S3(a0)
	sd s4, CONTEXT_OFFSET_S4(a0)
	sd s5, CONTEXT_OFFSET_S5(a0)
	sd s6, CONTEXT_OFFSET_S6(a0)
	sd s7, CONTEXT_OFFSET_S7(a0)
	sd s8, CONTEXT_OFFSET_S8(a0)
	sd s9, CONTEXT_OFFSET_S9(a0)
	sd s10, CONTEXT_OFFSET_S10(a0)
	sd s11, CONTEXT_OFFSET_S11(a0)

	sd ra, CONTEXT_OFFSET_PC(a0)
	sd sp, CONTEXT_OFFSET_SP(a0)

	# context_save returns 1
	li a0, 1
	ret
FUNCTION_END(context_save_arch)

FUNCTION_BEGIN(context_restore_arch)
	ld gp, CONTEXT_OFFSET_GP(a0)
	ld tp, CONTEXT_OFFSET_TP(a0)
	ld s0, CONTEXT_OFFSET_S0(a0)
	ld s1, CONTEXT_OFFSET_S1(a0)
	ld s2, CONTEXT_OFFSET_S2(a0)
	ld s3, CONTEXT_OFFSET_S3(a0)
	ld s4, CONTEXT_OFFSET_S4(a0)
	ld s5, CONTEXT_OFFSET_S5(a0)
	ld s6, CONTEXT_OFFSET_S6(a0)
	ld s7, CONTEXT_OFFSET_S7(a0)
	ld s8, CONTEXT_OFFSET_S8(a0)
	ld s9, CONTEXT_OFFSET_S9(a0)
	ld s10, CONTEXT_OFFSET_S10(a0)
	ld s11, CONTEXT_OFFSET_S11(a0)

	ld ra, CONTEXT_OFFSET_PC(a0)
	ld sp, CONTEXT_OFFSET_SP(a0)

	# context_restore returns 0
	li a0, 0
	ret
FUNCTION_END(context_restore_arch)
