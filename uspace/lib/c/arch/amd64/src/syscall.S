/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

## Make a system call.
#
# @param rdi		First argument.
# @param rsi		Second argument.
# @param rdx		Third argument.
# @param rcx		Fourth argument.
# @param r8		Fifth argument.
# @param r9		Sixth argument.
# @param 8(%rsp)	Syscall number.
#
# @return		The return value will be stored in RAX.
#
FUNCTION_BEGIN(__syscall)
	#
	# Move the syscall number into RAX.
	movslq 8(%rsp), %rax

	#
	# Save RCX, the 4th argument, aside because RCX gets rewritten by the
	# SYSCALL instruction.
	#
	movq %rcx, %r10
	syscall
	ret
FUNCTION_END(__syscall)

