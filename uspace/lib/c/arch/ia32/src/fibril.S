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
# Save CPU context to the context_t variable
# pointed by the 1st argument. Returns 0 in EAX.
#
FUNCTION_BEGIN(__context_save)
	movl 0(%esp), %eax  # the caller's return %eip
	movl 4(%esp), %edx  # address of the context variable to save context to

	# save registers to the context structure
	movl %esp, __CONTEXT_OFFSET_SP(%edx)	# %esp -> ctx->sp
	movl %eax, __CONTEXT_OFFSET_PC(%edx)	# %eip -> ctx->pc
	movl %ebx, __CONTEXT_OFFSET_EBX(%edx)	# %ebx -> ctx->ebx
	movl %esi, __CONTEXT_OFFSET_ESI(%edx)	# %esi -> ctx->esi
	movl %edi, __CONTEXT_OFFSET_EDI(%edx)	# %edi -> ctx->edi
	movl %ebp, __CONTEXT_OFFSET_EBP(%edx)	# %ebp -> ctx->ebp

	# save TLS
	movl %gs:0, %eax
	movl %eax, __CONTEXT_OFFSET_TLS(%edx)	# tls -> ctx->tls

	xorl %eax, %eax		# __context_save returns 0
	ret
FUNCTION_END(__context_save)

## Restore saved CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns second argument in EAX.
#
FUNCTION_BEGIN(__context_restore)
	movl 4(%esp), %eax  # address of the context variable to restore context from
	movl 8(%esp), %ecx  # return value

	# restore registers from the context structure
	movl __CONTEXT_OFFSET_SP(%eax),%esp	# ctx->sp -> %esp
	movl __CONTEXT_OFFSET_PC(%eax),%edx	# ctx->pc -> \pc
	movl __CONTEXT_OFFSET_EBX(%eax),%ebx	# ctx->ebx -> %ebx
	movl __CONTEXT_OFFSET_ESI(%eax),%esi	# ctx->esi -> %esi
	movl __CONTEXT_OFFSET_EDI(%eax),%edi	# ctx->edi -> %edi
	movl __CONTEXT_OFFSET_EBP(%eax),%ebp	# ctx->ebp -> %ebp

	movl %edx, 0(%esp)  # ctx->pc -> saver's return %eip

	# set thread local storage
	movl __CONTEXT_OFFSET_TLS(%eax), %edx	# Set arg1 to TLS addr
	movl %edx, %gs:0

	movl %ecx, %eax
	ret
FUNCTION_END(__context_restore)

