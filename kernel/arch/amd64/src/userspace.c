/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup amd64
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
		   [stack_top] "r" ((uint8_t *) kernel_uarg->uspace_stack +
		       kernel_uarg->uspace_stack_size),
		   [rflags] "r" (rflags),
		   [utext_des] "i" (GDT_SELECTOR(UTEXT_DES) | PL_USER),
		   [entry] "r" (kernel_uarg->uspace_entry),
		   [uarg] "r" (kernel_uarg->uspace_uarg)
		: "rax"
	);

	/* Unreachable */
	while (true);
}

/** @}
 */
