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

#include <print.h>
#include <test.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <arch/mm/tlb.h>
#include <typedefs.h>
#include <debug.h>

extern void tlb_invalidate_all(void);
extern void tlb_invalidate_pages(asid_t asid, uintptr_t va, size_t cnt);

const char *test_purge1(void)
{
	tlb_entry_t entryi;
	tlb_entry_t entryd;

	int i;

	entryd.word[0] = 0;
	entryd.word[1] = 0;

	entryd.p = true;                 /* present */
	entryd.ma = MA_WRITEBACK;
	entryd.a = true;                 /* already accessed */
	entryd.d = true;                 /* already dirty */
	entryd.pl = PL_KERNEL;
	entryd.ar = AR_READ | AR_WRITE;
	entryd.ppn = 0;
	entryd.ps = PAGE_WIDTH;

	entryi.word[0] = 0;
	entryi.word[1] = 0;

	entryi.p = true;                 /* present */
	entryi.ma = MA_WRITEBACK;
	entryi.a = true;                 /* already accessed */
	entryi.d = true;                 /* already dirty */
	entryi.pl = PL_KERNEL;
	entryi.ar = AR_READ | AR_EXECUTE;
	entryi.ppn = 0;
	entryi.ps = PAGE_WIDTH;

	for (i = 0; i < 100; i++) {
		itc_mapping_insert(0 + i * (1 << PAGE_WIDTH), 8, entryi);
		dtc_mapping_insert(0 + i * (1 << PAGE_WIDTH), 9, entryd);
	}

	tlb_invalidate_pages(8, 0x0c000, 14);

	/* tlb_invalidate_all(); */

	return NULL;
}
