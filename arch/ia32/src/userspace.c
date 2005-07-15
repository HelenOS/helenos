/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <userspace.h>
#include <arch/pm.h>
#include <arch/types.h>
#include <arch.h>
#include <proc/thread.h>
#include <mm/vm.h>


/** Enter userspace
 *
 * Change CPU protection level to 3, enter userspace.
 *
 */
void userspace(void)
{
	pri_t pri;
	
	pri = cpu_priority_high();

	__asm__ volatile (""
		"pushl %0\n"
		"pushl %1\n"
		"pushl %2\n"
		"pushl %3\n"
		"pushl %4\n"
		"iret"
		: : "i" (selector(UDATA_DES) | PL_USER), "i" (USTACK_ADDRESS+(THREAD_STACK_SIZE-1)), "r" (pri), "i" (selector(UTEXT_DES) | PL_USER), "i" (UTEXT_ADDRESS));
	
	/* Unreachable */
	for(;;);
}
