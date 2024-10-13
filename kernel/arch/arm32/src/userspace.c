/*
 * Copyright (c) 2007 Petr Stepan, Pavel Jancik
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

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Userspace switch.
 */

#include <stdbool.h>
#include <userspace.h>
#include <arch/ras.h>

/** Struct for holding all general purpose registers.
 *
 *  Used to set registers when going to userspace.
 */
typedef struct {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t sp;
	uint32_t lr;
	uint32_t pc;
} ustate_t;

uintptr_t arch_get_initial_sp(uintptr_t stack_base, uintptr_t stack_size)
{
	return stack_base + stack_size;
}

/** Change processor mode
 *
 * @param kernel_uarg Userspace settings (entry point, stack, ...).
 *
 */
void userspace(uintptr_t pc, uintptr_t sp)
{
	volatile ustate_t ustate = { };

	/* pass the RAS page address in %r2 */
	ustate.r2 = (uintptr_t) ras_page;

	ustate.sp = sp;
	ustate.pc = pc;

	/* status register in user mode */
	ipl_t user_mode = current_status_reg_read() &
	    (~STATUS_REG_MODE_MASK | USER_MODE);

	/* set user mode, set registers, jump */
	asm volatile (
	    "mov sp, %[ustate]\n"
	    "msr spsr_c, %[user_mode]\n"
	    "ldmfd sp, {r0-r12, sp, lr}^\n"
	    "nop\n"		/* Cannot access sp immediately after ldm(2) */
	    "add sp, sp, #(15*4)\n"
	    "ldmfd sp!, {pc}^\n"
	    :: [ustate] "r" (&ustate), [user_mode] "r" (user_mode)
	);

	unreachable();
}

/** @}
 */
