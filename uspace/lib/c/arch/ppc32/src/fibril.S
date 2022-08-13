/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

.text

#include <abi/asmtool.h>
#include <libarch/regname.h>
#include <libarch/fibril_context.h>

FUNCTION_BEGIN(__context_save)
	stw sp, __CONTEXT_OFFSET_SP(r3)
	stw r2, __CONTEXT_OFFSET_TLS(r3)
	stw r13, __CONTEXT_OFFSET_R13(r3)
	stw r14, __CONTEXT_OFFSET_R14(r3)
	stw r15, __CONTEXT_OFFSET_R15(r3)
	stw r16, __CONTEXT_OFFSET_R16(r3)
	stw r17, __CONTEXT_OFFSET_R17(r3)
	stw r18, __CONTEXT_OFFSET_R18(r3)
	stw r19, __CONTEXT_OFFSET_R19(r3)
	stw r20, __CONTEXT_OFFSET_R20(r3)
	stw r21, __CONTEXT_OFFSET_R21(r3)
	stw r22, __CONTEXT_OFFSET_R22(r3)
	stw r23, __CONTEXT_OFFSET_R23(r3)
	stw r24, __CONTEXT_OFFSET_R24(r3)
	stw r25, __CONTEXT_OFFSET_R25(r3)
	stw r26, __CONTEXT_OFFSET_R26(r3)
	stw r27, __CONTEXT_OFFSET_R27(r3)
	stw r28, __CONTEXT_OFFSET_R28(r3)
	stw r29, __CONTEXT_OFFSET_R29(r3)
	stw r30, __CONTEXT_OFFSET_R30(r3)
	stw r31, __CONTEXT_OFFSET_R31(r3)

	mflr r4
	stw r4, __CONTEXT_OFFSET_PC(r3)

	mfcr r4
	stw r4, __CONTEXT_OFFSET_CR(r3)

	# __context_save returns 0
	li r3, 0
	blr
FUNCTION_END(__context_save)

FUNCTION_BEGIN(__context_restore)
	lwz sp, __CONTEXT_OFFSET_SP(r3)
	lwz r2, __CONTEXT_OFFSET_TLS(r3)
	lwz r13, __CONTEXT_OFFSET_R13(r3)
	lwz r14, __CONTEXT_OFFSET_R14(r3)
	lwz r15, __CONTEXT_OFFSET_R15(r3)
	lwz r16, __CONTEXT_OFFSET_R16(r3)
	lwz r17, __CONTEXT_OFFSET_R17(r3)
	lwz r18, __CONTEXT_OFFSET_R18(r3)
	lwz r19, __CONTEXT_OFFSET_R19(r3)
	lwz r20, __CONTEXT_OFFSET_R20(r3)
	lwz r21, __CONTEXT_OFFSET_R21(r3)
	lwz r22, __CONTEXT_OFFSET_R22(r3)
	lwz r23, __CONTEXT_OFFSET_R23(r3)
	lwz r24, __CONTEXT_OFFSET_R24(r3)
	lwz r25, __CONTEXT_OFFSET_R25(r3)
	lwz r26, __CONTEXT_OFFSET_R26(r3)
	lwz r27, __CONTEXT_OFFSET_R27(r3)
	lwz r28, __CONTEXT_OFFSET_R28(r3)
	lwz r29, __CONTEXT_OFFSET_R29(r3)
	lwz r30, __CONTEXT_OFFSET_R30(r3)
	lwz r31, __CONTEXT_OFFSET_R31(r3)

	lwz r5, __CONTEXT_OFFSET_CR(r3)
	mtcr r5

	lwz r5, __CONTEXT_OFFSET_PC(r3)
	mtlr r5

	# __context_restore returns second argument
	mr r3, r4
	blr
FUNCTION_END(__context_restore)
