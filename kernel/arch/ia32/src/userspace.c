/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#include <userspace.h>
#include <arch/pm.h>
#include <typedefs.h>
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
	ipl_t ipl = interrupts_disable();
	
	asm volatile (
		/*
		 * Clear nested task flag.
		 */
		"pushfl\n"
		"pop %%eax\n"
		"and $0xffffbfff, %%eax\n"
		"push %%eax\n"
		"popfl\n"
		
		/* Set up GS register (TLS) */
		"movl %[tls_des], %%gs\n"
		
		"pushl %[udata_des]\n"
		"pushl %[stack_top]\n"
		"pushl %[ipl]\n"
		"pushl %[utext_des]\n"
		"pushl %[entry]\n"
		"movl %[uarg], %%eax\n"
		
		/* %edi is defined to hold pcb_ptr - set it to 0 */
		"xorl %%edi, %%edi\n"
		
		"iret\n"
		:
		: [udata_des] "i" (GDT_SELECTOR(UDATA_DES) | PL_USER),
		  [stack_top] "r" ((uint8_t *) kernel_uarg->uspace_stack +
		      kernel_uarg->uspace_stack_size),
		  [ipl] "r" (ipl),
		  [utext_des] "i" (GDT_SELECTOR(UTEXT_DES) | PL_USER),
		  [entry] "r" (kernel_uarg->uspace_entry),
		  [uarg] "r" (kernel_uarg->uspace_uarg),
		  [tls_des] "r" (GDT_SELECTOR(TLS_DES))
		: "eax");
	
	/* Unreachable */
	while (1);
}

/** @}
 */
