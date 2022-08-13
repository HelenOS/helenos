/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <arch/mm/tlb.h>
#include <typedefs.h>

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
