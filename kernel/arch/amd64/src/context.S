/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/context_struct.h>
#include <arch/vreg.h>

.text

## Save current CPU context
#
# Save CPU context to context_t variable
# pointed by the 1st argument. Returns 1 in EAX.
#
FUNCTION_BEGIN(context_save_arch)
	movq (%rsp), %rdx     # the caller's return %rip

	movq %rdx, CONTEXT_OFFSET_PC(%rdi)
	movq %rsp, CONTEXT_OFFSET_SP(%rdi)

	movq %rbx, CONTEXT_OFFSET_RBX(%rdi)
	movq %rbp, CONTEXT_OFFSET_RBP(%rdi)
	movq %r12, CONTEXT_OFFSET_R12(%rdi)
	movq %r13, CONTEXT_OFFSET_R13(%rdi)
	movq %r14, CONTEXT_OFFSET_R14(%rdi)
	movq %r15, CONTEXT_OFFSET_R15(%rdi)

#ifdef MEMORY_MODEL_large
	movabsq $vreg_ptr, %rsi
	movq (%rsi), %rsi
#else
	movq vreg_ptr, %rsi
#endif
	movq %fs:VREG_TP(%rsi), %rsi
	movq %rsi, CONTEXT_OFFSET_TP(%rdi)

	xorl %eax, %eax       # context_save returns 1
	incl %eax
	ret
FUNCTION_END(context_save_arch)


## Restore current CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns 0 in EAX.
#
FUNCTION_BEGIN(context_restore_arch)
	movq CONTEXT_OFFSET_R15(%rdi), %r15
	movq CONTEXT_OFFSET_R14(%rdi), %r14
	movq CONTEXT_OFFSET_R13(%rdi), %r13
	movq CONTEXT_OFFSET_R12(%rdi), %r12
	movq CONTEXT_OFFSET_RBP(%rdi), %rbp
	movq CONTEXT_OFFSET_RBX(%rdi), %rbx

	movq CONTEXT_OFFSET_SP(%rdi), %rsp

	movq CONTEXT_OFFSET_PC(%rdi), %rdx
	movq %rdx, (%rsp)

	movq CONTEXT_OFFSET_TP(%rdi), %rcx
#ifdef MEMORY_MODEL_large
	movabsq $vreg_ptr, %rsi
	movq (%rsi), %rsi
#else
	movq vreg_ptr, %rsi
#endif
	movq %rcx, %fs:VREG_TP(%rsi)

	xorl %eax, %eax       # context_restore returns 0
	ret
FUNCTION_END(context_restore_arch)

