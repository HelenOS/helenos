/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#include <userspace.h>
#include <arch/cpu.h>
#include <arch/pm.h>
#include <stdbool.h>
#include <stdint.h>
#include <arch.h>
#include <abi/proc/uarg.h>
#include <mm/as.h>

/** Enter userspace
 *
 * Change CPU protection level to 3, enter userspace.
 *
 */
void userspace(uspace_arg_t *kernel_uarg)
{
	uint64_t rflags = read_rflags();

	rflags &= ~RFLAGS_NT;
	rflags |= RFLAGS_IF;

	asm volatile (
	    "pushq %[udata_des]\n"
	    "pushq %[stack_top]\n"
	    "pushq %[rflags]\n"
	    "pushq %[utext_des]\n"
	    "pushq %[entry]\n"
	    "movq %[uarg], %%rax\n"

	    /* %rdi is defined to hold pcb_ptr - set it to 0 */
	    "xorq %%rdi, %%rdi\n"
	    "iretq\n"
	    :: [udata_des] "i" (GDT_SELECTOR(UDATA_DES) | PL_USER),
	      [stack_top] "r" (kernel_uarg->uspace_stack +
	      kernel_uarg->uspace_stack_size),
	      [rflags] "r" (rflags),
	      [utext_des] "i" (GDT_SELECTOR(UTEXT_DES) | PL_USER),
	      [entry] "r" (kernel_uarg->uspace_entry),
	      [uarg] "r" (kernel_uarg->uspace_uarg)
	    : "rax"
	);

	unreachable();
}

/** @}
 */
