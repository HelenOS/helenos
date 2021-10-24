/*
 * Copyright (c) 2015 Petr Pavlu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <abi/asmtool.h>
#include <arch/exception.h>
#include <arch/istate_struct.h>

.text

FUNCTION_BEGIN(memcpy_from_uspace)
FUNCTION_BEGIN(memcpy_to_uspace)
	/* Simple (un-optimized) memcpy(). */
	cbz x2, 2f
	mov x3, x0
1:
	ldrb w4, [x1], #1
	strb w4, [x3], #1
	subs x2, x2, #1
	b.ne 1b

2:
	ret
FUNCTION_END(memcpy_from_uspace)
FUNCTION_END(memcpy_to_uspace)

FUNCTION_BEGIN(memcpy_from_uspace_failover_address)
FUNCTION_BEGIN(memcpy_to_uspace_failover_address)
	mov x0, #0
	ret
FUNCTION_END(memcpy_from_uspace_failover_address)
FUNCTION_END(memcpy_to_uspace_failover_address)

/** Flush instruction caches
 *
 * @param x0 Starting address of the flushing.
 * @param x1 Number of bytes to flush.
 *
 */
FUNCTION_BEGIN(smc_coherence)
	/* Initialize loop */
	mov x9, x0
	mov x10, xzr

	__dc_loop:
		/* Data or Unified Cache Line Clean */
		dc cvau, x9
		add x9, x9, #4
		add x10, x10, #4
		cmp x10, x1
		blo __dc_loop

	dsb ish

	/* Initialize loop */
	mov x9, x0
	mov x10, xzr

	__ic_loop:
		/* Instruction Cache Line Invalidate */
		ic ivau, x9
		add x9, x9, #4
		add x10, x10, #4
		cmp x10, x1
		blo __ic_loop

	dsb ish
	isb
	ret
FUNCTION_END(smc_coherence)

/* Static checks for the istate_t save/load. */
#if ISTATE_OFFSET_X0 + 8 != ISTATE_OFFSET_X1
#error x0 and x1 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X2 + 8 != ISTATE_OFFSET_X3
#error x2 and x3 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X4 + 8 != ISTATE_OFFSET_X5
#error x4 and x5 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X6 + 8 != ISTATE_OFFSET_X7
#error x6 and x7 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X8 + 8 != ISTATE_OFFSET_X9
#error x8 and x9 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X10 + 8 != ISTATE_OFFSET_X11
#error x10 and x11 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X12 + 8 != ISTATE_OFFSET_X13
#error x12 and x13 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X14 + 8 != ISTATE_OFFSET_X15
#error x14 and x15 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X16 + 8 != ISTATE_OFFSET_X17
#error x16 and x17 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X18 + 8 != ISTATE_OFFSET_X19
#error x18 and x19 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X20 + 8 != ISTATE_OFFSET_X21
#error x20 and x21 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X22 + 8 != ISTATE_OFFSET_X23
#error x22 and x23 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X24 + 8 != ISTATE_OFFSET_X25
#error x24 and x25 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X26 + 8 != ISTATE_OFFSET_X27
#error x26 and x27 are not successive in istate_t
#endif
#if ISTATE_OFFSET_X28 + 8 != ISTATE_OFFSET_X29
#error x28 and x29 are not successive in istate_t
#endif
#if ISTATE_OFFSET_SPSR + 8 != ISTATE_OFFSET_SP
#error spsr and sp are not successive in istate_t
#endif
#if ISTATE_OFFSET_PC + 8 != ISTATE_OFFSET_TPIDR
#error pc and tpidr are not successive in istate_t
#endif

/* Exception vector. */
.macro handler i
handler_\i:
	/*
	 * Initial code for each handler, at maximum 128 bytes (32
	 * instructions).
	 */

	/* Save current state. */
	sub sp, sp, #ISTATE_SIZE                /* 0x00 */
	stp x0, x1, [sp, #ISTATE_OFFSET_X0]     /* 0x04 */
	stp x2, x3, [sp, #ISTATE_OFFSET_X2]     /* 0x08 */
	stp x4, x5, [sp, #ISTATE_OFFSET_X4]     /* 0x0c */
	stp x6, x7, [sp, #ISTATE_OFFSET_X6]     /* 0x10 */
	stp x8, x9, [sp, #ISTATE_OFFSET_X8]     /* 0x14 */
	stp x10, x11, [sp, #ISTATE_OFFSET_X10]  /* 0x18 */
	stp x12, x13, [sp, #ISTATE_OFFSET_X12]  /* 0x1c */
	stp x14, x15, [sp, #ISTATE_OFFSET_X14]  /* 0x20 */
	stp x16, x17, [sp, #ISTATE_OFFSET_X16]  /* 0x24 */
	stp x18, x19, [sp, #ISTATE_OFFSET_X18]  /* 0x28 */
	stp x20, x21, [sp, #ISTATE_OFFSET_X20]  /* 0x2c */
	stp x22, x23, [sp, #ISTATE_OFFSET_X22]  /* 0x30 */
	stp x24, x25, [sp, #ISTATE_OFFSET_X24]  /* 0x34 */
	stp x26, x27, [sp, #ISTATE_OFFSET_X26]  /* 0x38 */
	stp x28, x29, [sp, #ISTATE_OFFSET_X28]  /* 0x3c */
	str x30, [sp, #ISTATE_OFFSET_X30]       /* 0x40 */

	mrs x0, spsr_el1                        /* 0x44 */
	mrs x1, sp_el0                          /* 0x48 */
	stp x0, x1, [sp, #ISTATE_OFFSET_SPSR]   /* 0x4c */

	mrs x0, elr_el1                         /* 0x50 */
	mrs x1, tpidr_el0                       /* 0x54 */
	stp x0, x1, [sp, #ISTATE_OFFSET_PC]     /* 0x58 */

	mov x0, #\i                             /* 0x5c */
	mov x1, sp                              /* 0x60 */
	bl exc_dispatch                         /* 0x64 */

	/* Restore previous state. */
	ldp x0, x1, [sp, #ISTATE_OFFSET_SPSR]   /* 0x68 */
	msr spsr_el1, x0                        /* 0x6c */
	msr sp_el0, x1                          /* 0x70 */

	ldp x0, x1, [sp, #ISTATE_OFFSET_PC]     /* 0x74 */
	msr elr_el1, x0                         /* 0x78 */
	b exc_restore_end                       /* 0x7c */
.endm

exc_restore_end:
	/* Restore remaining registers and return from the exception handler. */
	msr tpidr_el0, x1
	ldp x0, x1, [sp, #ISTATE_OFFSET_X0]
	ldp x2, x3, [sp, #ISTATE_OFFSET_X2]
	ldp x4, x5, [sp, #ISTATE_OFFSET_X4]
	ldp x6, x7, [sp, #ISTATE_OFFSET_X6]
	ldp x8, x9, [sp, #ISTATE_OFFSET_X8]
	ldp x10, x11, [sp, #ISTATE_OFFSET_X10]
	ldp x12, x13, [sp, #ISTATE_OFFSET_X12]
	ldp x14, x15, [sp, #ISTATE_OFFSET_X14]
	ldp x16, x17, [sp, #ISTATE_OFFSET_X16]
	ldp x18, x19, [sp, #ISTATE_OFFSET_X18]
	ldp x20, x21, [sp, #ISTATE_OFFSET_X20]
	ldp x22, x23, [sp, #ISTATE_OFFSET_X22]
	ldp x24, x25, [sp, #ISTATE_OFFSET_X24]
	ldp x26, x27, [sp, #ISTATE_OFFSET_X26]
	ldp x28, x29, [sp, #ISTATE_OFFSET_X28]
	ldr x30, [sp, #ISTATE_OFFSET_X30]
	add sp, sp, #ISTATE_SIZE
	eret

.align 11
SYMBOL(exc_vector)
.org exc_vector + 0x000
	handler EXC_CURRENT_EL_SP_SEL0_SYNCH
.org exc_vector + 0x080
	handler EXC_CURRENT_EL_SP_SEL0_IRQ
.org exc_vector + 0x100
	handler EXC_CURRENT_EL_SP_SEL0_FIQ
.org exc_vector + 0x180
	handler EXC_CURRENT_EL_SP_SEL0_SERROR
.org exc_vector + 0x200
	handler EXC_CURRENT_EL_SP_SELX_SYNCH
.org exc_vector + 0x280
	handler EXC_CURRENT_EL_SP_SELX_IRQ
.org exc_vector + 0x300
	handler EXC_CURRENT_EL_SP_SELX_FIQ
.org exc_vector + 0x380
	handler EXC_CURRENT_EL_SP_SELX_SERROR
.org exc_vector + 0x400
	handler EXC_LOWER_EL_AARCH64_SYNCH
.org exc_vector + 0x480
	handler EXC_LOWER_EL_AARCH64_IRQ
.org exc_vector + 0x500
	handler EXC_LOWER_EL_AARCH64_FIQ
.org exc_vector + 0x580
	handler EXC_LOWER_EL_AARCH64_SERROR
.org exc_vector + 0x600
	handler EXC_LOWER_EL_AARCH32_SYNCH
.org exc_vector + 0x680
	handler EXC_LOWER_EL_AARCH32_IRQ
.org exc_vector + 0x700
	handler EXC_LOWER_EL_AARCH32_FIQ
.org exc_vector + 0x780
	handler EXC_LOWER_EL_AARCH32_SERROR
.org exc_vector + 0x800
