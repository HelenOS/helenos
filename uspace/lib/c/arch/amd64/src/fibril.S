/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <libarch/fibril_context.h>

.text

## Save current CPU context
#
# Save CPU context to context_t variable
# pointed by the 1st argument. Returns 0 in RAX.
#
FUNCTION_BEGIN(__context_save)
	movq (%rsp), %rdx     # the caller's return %eip

	# in %rdi is passed 1st argument
	movq %rdx, __CONTEXT_OFFSET_PC(%rdi)
	movq %rsp, __CONTEXT_OFFSET_SP(%rdi)

	movq %rbx, __CONTEXT_OFFSET_RBX(%rdi)
	movq %rbp, __CONTEXT_OFFSET_RBP(%rdi)
	movq %r12, __CONTEXT_OFFSET_R12(%rdi)
	movq %r13, __CONTEXT_OFFSET_R13(%rdi)
	movq %r14, __CONTEXT_OFFSET_R14(%rdi)
	movq %r15, __CONTEXT_OFFSET_R15(%rdi)

	movq %fs:0, %rax
	movq %rax, __CONTEXT_OFFSET_TLS(%rdi)

	xorq %rax, %rax                      # __context_save returns 0
	ret
FUNCTION_END(__context_save)

## Restore current CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns second argument in RAX.
#
FUNCTION_BEGIN(__context_restore)
	movq __CONTEXT_OFFSET_R15(%rdi), %r15
	movq __CONTEXT_OFFSET_R14(%rdi), %r14
	movq __CONTEXT_OFFSET_R13(%rdi), %r13
	movq __CONTEXT_OFFSET_R12(%rdi), %r12
	movq __CONTEXT_OFFSET_RBP(%rdi), %rbp
	movq __CONTEXT_OFFSET_RBX(%rdi), %rbx

	movq __CONTEXT_OFFSET_SP(%rdi), %rsp   # ctx->sp -> %rsp

	movq __CONTEXT_OFFSET_PC(%rdi), %rdx

	movq %rdx,(%rsp)

	movq __CONTEXT_OFFSET_TLS(%rdi), %rdi
	movq %rdi, %fs:0

	movq %rsi, %rax                      # __context_restore returns second argument
	ret
FUNCTION_END(__context_restore)

