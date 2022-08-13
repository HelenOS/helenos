/*
 * SPDX-FileCopyrightText: 2007 Petr Stepan, Pavel Jancik
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/** Change processor mode
 *
 * @param kernel_uarg Userspace settings (entry point, stack, ...).
 *
 */
void userspace(uspace_arg_t *kernel_uarg)
{
	volatile ustate_t ustate;

	/* set first parameter */
	ustate.r0 = kernel_uarg->uspace_uarg;

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
	ustate.sp = kernel_uarg->uspace_stack +
	    kernel_uarg->uspace_stack_size;

	/* set where uspace execution starts */
	ustate.pc = kernel_uarg->uspace_entry;

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
