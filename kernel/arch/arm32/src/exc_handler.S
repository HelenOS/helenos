#
# Copyright (c) 2009 Vineeth Pillai
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#include <abi/asmtool.h>

.text

# Switches to kernel stack and saves all registers there.
#
#  The stack frame created by the function looks like:
#
#              |_________________|
#              |                 |
#              |     SPSR        |
#              |                 |
#              |_________________|
#              | Stack Pointer   |
#              |      of         |
#              | Previous Mode   |
#              |_________________|
#              | Return address  |
#              |      of         |
#              | Previous Mode   |
#              |_________________|
#              |   R0  - R12     |
#              |      of         |
#              | Previous Mode   |
#              |_________________|
#              | Return address  |
#              |     from        |
#              |Exception Handler|
#              |_________________|
#              |                 |
#
#

.macro SAVE_REGS_TO_STACK
	stmfd r13!, {r0-r3}
	mov r3, sp
	add sp, sp, #16
	mrs r1, cpsr
	bic r1, r1, #0x1f
	mrs r2, spsr
	and r0, r2, #0x1f
	cmp r0, #0x10
	bne 1f

	# prev mode was usermode
	mov r0, lr

	# Switch to supervisor mode
	orr r1, r1, #0x13
	msr cpsr_c, r1

	# Load sp with [supervisor_sp]
	ldr r13, =supervisor_sp
	ldr r13, [r13]

	# Populate the stack frame
	msr spsr, r2
	mov lr, r0
	stmfd r13!, {lr}
	stmfd r13!, {r4-r12}
	ldmfd r3!, {r4-r7}
	stmfd r13!, {r4-r7}
	mov r4, r13
	stmfd r4, {r13, lr}^
	nop			/* Cannot access r13 immediately after stm(2) */
	sub r13, r13, #8
	stmfd r13!, {r2}

	# Stop stack traces here
	mov fp, #0

	b 2f

	# mode was not usermode
1:
	# Switch to previous mode which is undoubtedly the supervisor mode
	orr r1, r1, r0
	mov r0, lr
	msr cpsr_c, r1

	# Populate the stack frame
	mov r1, sp
	stmfd r13!, {r0}
	stmfd r13!, {r4-r12}

	# Store r0-r3 in r4-r7 and then push it on to stack
	ldmfd r3!, {r4-r7}
	stmfd r13!, {r4-r7}

	# Push return address and stack pointer on to stack
	stmfd r13!, {lr}
	stmfd r13!, {r1}
	mov lr, r0
	msr spsr, r2
	stmfd r13!, {r2}
2:
	sub sp, sp, #4
.endm

.macro LOAD_REGS_FROM_STACK
	add sp, sp, #4
	ldmfd r13!, {r0}
	msr spsr, r0
	and r0, r0, #0x1f
	cmp r0, #0x10
	bne 1f

	# return to user mode
	mov r0, r13
	ldmfd r0, {r13, lr}^
	nop			/* Cannot access r13 immediately after ldm(2) */
	add r13, r13, #8
	b 2f

	# return to non-user mode
1:
	ldmfd r13!, {r1, lr}

2:
	ldmfd r13!, {r0-r12, pc}^
.endm

SYMBOL(reset_exception_entry)
	SAVE_REGS_TO_STACK
	mov r0, #0
	mov r1, r13
	bl ras_check
	LOAD_REGS_FROM_STACK

SYMBOL(irq_exception_entry)
	sub lr, lr, #4
	SAVE_REGS_TO_STACK
	mov r0, #5
	mov r1, r13
	bl ras_check
	LOAD_REGS_FROM_STACK

SYMBOL(fiq_exception_entry)
	sub lr, lr, #4
	SAVE_REGS_TO_STACK
	mov r0, #6
	mov r1, r13
	bl ras_check
	LOAD_REGS_FROM_STACK

SYMBOL(undef_instr_exception_entry)
	SAVE_REGS_TO_STACK
	mov r0, #1
	mov r1, r13
	bl ras_check
	LOAD_REGS_FROM_STACK

SYMBOL(prefetch_abort_exception_entry)
	sub lr, lr, #4
	SAVE_REGS_TO_STACK
	mov r0, #3
	mov r1, r13
	bl ras_check
	LOAD_REGS_FROM_STACK

SYMBOL(data_abort_exception_entry)
	sub lr, lr, #8
	SAVE_REGS_TO_STACK
	mov r0, #4
	mov r1, r13
	bl ras_check
	LOAD_REGS_FROM_STACK

SYMBOL(swi_exception_entry)
	ldr r13, =exc_stack
	SAVE_REGS_TO_STACK
	mov r0, #2
	mov r1, r13
	bl ras_check
	LOAD_REGS_FROM_STACK

