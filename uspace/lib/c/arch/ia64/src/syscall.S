/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

/*
 * IA-64 Software Conventions and Runtime Architecture Guide mandates that
 * the zero-extended imm21 operand of the BREAK instruction is used as follows:
 *
 * MSB | Purpose
 * ----+--------
 * 000 | Reserved for architected software interrupts
 * 001 | Available for application use as software interrupts
 * 01x | Reserved for debugger breakpoints
 * 1xx | Reserved for definition by each ABI
 */
#define SYSCALL_IMM	0x40000

FUNCTION_BEGIN(__syscall)
	alloc r14 = ar.pfs, 7, 0, 0, 0 ;;
	break SYSCALL_IMM
	mov ar.pfs = r14 ;;
	br.ret.sptk.many b0
FUNCTION_END(__syscall)
