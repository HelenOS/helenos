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

/** @addtogroup arm32
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

/** Change processor mode
 *
 * @param kernel_uarg Userspace settings (entry point, stack, ...).
 *
 */
void userspace(uspace_arg_t *kernel_uarg)
{
	volatile ustate_t ustate;

	/* set first parameter */
	ustate.r0 = (uintptr_t) kernel_uarg->uspace_uarg;

	/* %r1 is defined to hold pcb_ptr - set it to 0 */
	ustate.r1 = 0;

	/* pass the RAS page address in %r2 */
	ustate.r2 = (uintptr_t) ras_page;

	/* clear other registers */
	ustate.r3 = 0;
	ustate.r4 = 0;
	ustate.r5 = 0;
	ustate.r6 = 0;
	ustate.r7 = 0;
	ustate.r8 = 0;
	ustate.r9 = 0;
	ustate.r10 = 0;
	ustate.r11 = 0;
	ustate.r12 = 0;
	ustate.lr = 0;

	/* set user stack */
	ustate.sp = ((uint32_t) kernel_uarg->uspace_stack) +
	    kernel_uarg->uspace_stack_size;

	/* set where uspace execution starts */
	ustate.pc = (uintptr_t) kernel_uarg->uspace_entry;

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

	/* unreachable */
	while (true)
		;
}

/** @}
 */
