/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/context_struct.h>
#include <arch/asm/regname.h>

.text

FUNCTION_BEGIN(context_save_arch)
	stw sp, CONTEXT_OFFSET_SP(r3)
	stw r2, CONTEXT_OFFSET_R2(r3)
	stw r13, CONTEXT_OFFSET_R13(r3)
	stw r14, CONTEXT_OFFSET_R14(r3)
	stw r15, CONTEXT_OFFSET_R15(r3)
	stw r16, CONTEXT_OFFSET_R16(r3)
	stw r17, CONTEXT_OFFSET_R17(r3)
	stw r18, CONTEXT_OFFSET_R18(r3)
	stw r19, CONTEXT_OFFSET_R19(r3)
	stw r20, CONTEXT_OFFSET_R20(r3)
	stw r21, CONTEXT_OFFSET_R21(r3)
	stw r22, CONTEXT_OFFSET_R22(r3)
	stw r23, CONTEXT_OFFSET_R23(r3)
	stw r24, CONTEXT_OFFSET_R24(r3)
	stw r25, CONTEXT_OFFSET_R25(r3)
	stw r26, CONTEXT_OFFSET_R26(r3)
	stw r27, CONTEXT_OFFSET_R27(r3)
	stw r28, CONTEXT_OFFSET_R28(r3)
	stw r29, CONTEXT_OFFSET_R29(r3)
	stw r30, CONTEXT_OFFSET_R30(r3)
	stw r31, CONTEXT_OFFSET_R31(r3)

	mflr r4
	stw r4, CONTEXT_OFFSET_PC(r3)

	mfcr r4
	stw r4, CONTEXT_OFFSET_CR(r3)

	# context_save returns 1
	li r3, 1
	blr
FUNCTION_END(context_save_arch)

FUNCTION_BEGIN(context_restore_arch)
	lwz sp, CONTEXT_OFFSET_SP(r3)
	lwz r2, CONTEXT_OFFSET_R2(r3)
	lwz r13, CONTEXT_OFFSET_R13(r3)
	lwz r14, CONTEXT_OFFSET_R14(r3)
	lwz r15, CONTEXT_OFFSET_R15(r3)
	lwz r16, CONTEXT_OFFSET_R16(r3)
	lwz r17, CONTEXT_OFFSET_R17(r3)
	lwz r18, CONTEXT_OFFSET_R18(r3)
	lwz r19, CONTEXT_OFFSET_R19(r3)
	lwz r20, CONTEXT_OFFSET_R20(r3)
	lwz r21, CONTEXT_OFFSET_R21(r3)
	lwz r22, CONTEXT_OFFSET_R22(r3)
	lwz r23, CONTEXT_OFFSET_R23(r3)
	lwz r24, CONTEXT_OFFSET_R24(r3)
	lwz r25, CONTEXT_OFFSET_R25(r3)
	lwz r26, CONTEXT_OFFSET_R26(r3)
	lwz r27, CONTEXT_OFFSET_R27(r3)
	lwz r28, CONTEXT_OFFSET_R28(r3)
	lwz r29, CONTEXT_OFFSET_R29(r3)
	lwz r30, CONTEXT_OFFSET_R30(r3)
	lwz r31, CONTEXT_OFFSET_R31(r3)

	lwz r4, CONTEXT_OFFSET_CR(r3)
	mtcr r4

	lwz r4, CONTEXT_OFFSET_PC(r3)
	mtlr r4

	# context_restore returns 0
	li r3, 0
	blr
FUNCTION_END(context_restore_arch)
