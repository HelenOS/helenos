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
# Save CPU context to the context_t variable
# pointed by the 1st argument. Returns 1 in EAX.
#
FUNCTION_BEGIN(context_save_arch)
	movl 0(%esp), %eax	# save pc value into eax
	movl 4(%esp), %edx	# address of the context variable to save context to

	# save registers to given structure
	movl %esp, CONTEXT_OFFSET_SP(%edx)	# %esp -> ctx->sp
	movl %eax, CONTEXT_OFFSET_PC(%edx)	# %eip -> ctx->pc
	movl %ebx, CONTEXT_OFFSET_EBX(%edx)	# %ebx -> ctx->ebx
	movl %esi, CONTEXT_OFFSET_ESI(%edx)	# %esi -> ctx->esi
	movl %edi, CONTEXT_OFFSET_EDI(%edx)	# %edi -> ctx->edi
	movl %ebp, CONTEXT_OFFSET_EBP(%edx)	# %ebp -> ctx->ebp

	mov vreg_ptr, %ecx
	movl %gs:VREG_TP(%ecx), %ecx
	movl %ecx, CONTEXT_OFFSET_TP(%edx)

	xorl %eax, %eax		# context_save returns 1
	incl %eax
	ret
FUNCTION_END(context_save_arch)

## Restore saved CPU context
#
# Restore CPU context from context_t variable
# pointed by the 1st argument. Returns 0 in EAX.
#
FUNCTION_BEGIN(context_restore_arch)
	movl 4(%esp), %eax	# address of the context variable to restore context from

	# restore registers from given structure
	movl CONTEXT_OFFSET_SP(%eax), %esp	# ctx->sp -> %esp
	movl CONTEXT_OFFSET_PC(%eax), %edx	# ctx->pc -> \pc
	movl CONTEXT_OFFSET_EBX(%eax), %ebx	# ctx->ebx -> %ebx
	movl CONTEXT_OFFSET_ESI(%eax), %esi	# ctx->esi -> %esi
	movl CONTEXT_OFFSET_EDI(%eax), %edi	# ctx->edi -> %edi
	movl CONTEXT_OFFSET_EBP(%eax), %ebp	# ctx->ebp -> %ebp

	movl %edx, 0(%esp)	# put saved pc on stack

	mov vreg_ptr, %ecx
	movl CONTEXT_OFFSET_TP(%eax), %edx
	movl %edx, %gs:VREG_TP(%ecx)

	xorl %eax, %eax		# context_restore returns 0
	ret
