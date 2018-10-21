#
# Copyright (c) 2016 Martin Decky
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
#include <arch/arch.h>
#include <arch/mm.h>

#define MCOUNTEREN_CY_MASK  0x00000001
#define MCOUNTEREN_TM_MASK  0x00000002
#define MCOUNTEREN_IR_MASK  0x00000004

#define MSTATUS_MPP_MASK        0x00001800
#define MSTATUS_MPP_USER        0x00000000
#define MSTATUS_MPP_SUPERVISOR  0x00000800
#define MSTATUS_MPP_MACHINE     0x00001800

#define MSTATUS_SUM_MASK        0x00040000

#define SATP_PFN_MASK  0x00000fffffffffff

#define SATP_MODE_MASK  0xf000000000000000
#define SATP_MODE_BARE  0x0000000000000000
#define SATP_MODE_SV39  0x8000000000000000
#define SATP_MODE_SV48  0x9000000000000000

.section BOOTSTRAP

.org DEFAULT_MTVEC + TRAP_VECTOR_RESET
SYMBOL(start)
	/* Set up machine state */
	li x1, 0
	li x2, 0
	li x3, 0
	li x4, 0
	li x5, 0
	li x6, 0
	li x7, 0
	li x8, 0
	li x9, 0
	li x10, 0
	li x11, 0
	li x12, 0
	li x13, 0
	li x14, 0
	li x15, 0
	li x16, 0
	li x17, 0
	li x18, 0
	li x19, 0
	li x20, 0
	li x21, 0
	li x22, 0
	li x23, 0
	li x24, 0
	li x25, 0
	li x26, 0
	li x27, 0
	li x28, 0
	li x29, 0
	li x30, 0
	li x31, 0

	/* Set up stack, create stack frame */
	la sp, boot_stack + BOOT_STACK_SIZE
	addi sp, sp, -16

	j bootstrap

.text

FUNCTION_BEGIN(jump_to_kernel)
	/* Enable performance counters access in supervisor mode */
	csrsi mcounteren, MCOUNTEREN_CY_MASK | MCOUNTEREN_TM_MASK | MCOUNTEREN_IR_MASK

	/* Setup SV48 paging for supervisor mode */
	la t0, ptl_0
	srli t0, t0, 12

	li t1, SATP_PFN_MASK
	and t0, t0, t1

	li t1, SATP_MODE_SV48
	or t0, t0, t1

	csrw sptbr, t0

	/* Jump to supervisor mode */
	csrr t0, mstatus

	li t1, ~MSTATUS_MPP_MASK
	and t0, t0, t1

	/*
	 * TODO: Enable running with Supervisor User Mode
	 * access disabled.
	 */
	li t1, MSTATUS_MPP_SUPERVISOR | MSTATUS_SUM_MASK
	or t0, t0, t1

	csrw mstatus, t0

	/* Entry point address is in a1. */
	mv ra, a1
	csrw mepc, ra

	mret
FUNCTION_END(jump_to_kernel)

FUNCTION_BEGIN(halt)
	j halt
FUNCTION_END(halt)

.data

.align PAGE_WIDTH
SYMBOL(boot_stack)
	.space BOOT_STACK_SIZE

.section .pt, "aw", @progbits

.align PAGE_WIDTH
SYMBOL(ptl_0)
	.fill 256, 8, 0
	/* Identity mapping for [0; 512G) */
	.quad 0 + (PTL_DIRTY | PTL_ACCESSED | PTL_EXECUTABLE | PTL_WRITABLE | PTL_READABLE | PTL_VALID)
	.fill 255, 8, 0
