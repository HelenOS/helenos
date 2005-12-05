/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <arch/mm/page.h>
#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <arch/types.h>
#include <config.h>
#include <func.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <synch/spinlock.h>
#include <debug.h>
#include <memstr.h>
#include <print.h>

__address bootstrap_dba; 

void page_arch_init(void)
{
	__address dba;
	__address cur;

	if (config.cpu_active == 1) {
		dba = frame_alloc(FRAME_KA | FRAME_PANIC, 0);
		memsetb(dba, PAGE_SIZE, 0);

		bootstrap_dba = dba;
		
		/*
		 * PA2KA(identity) mapping for all frames until last_frame.
		 */
		for (cur = 0; cur < last_frame; cur += FRAME_SIZE)
			page_mapping_insert(PA2KA(cur), cur, PAGE_CACHEABLE, KA2PA(dba));

		trap_register(14, page_fault);
		write_cr3(KA2PA(dba));
	}
	else {
		/*
		 * Application processors need to create their own view of the
		 * virtual address space. Because of that, each AP copies
		 * already-initialized paging information from the bootstrap
		 * processor and adjusts it to fulfill its needs.
		 */

		dba = frame_alloc(FRAME_KA | FRAME_PANIC, 0);
		memcpy((void *)dba, (void *)bootstrap_dba , PAGE_SIZE);
		write_cr3(KA2PA(dba));
	}

	paging_on();
}
