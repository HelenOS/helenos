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

#ifndef __ia64_TLB_H__
#define __ia64_TLB_H__

#define tlb_arch_init()
#define tlb_print()

#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <arch/interrupt.h>
#include <arch/types.h>
#include <typedefs.h>

/** Data and instruction Translation Register indices. */
#define DTR_KERNEL	0
#define ITR_KERNEL	0
#define DTR_KSTACK	1

/** Portion of TLB insertion format data structure. */
union tlb_entry {
	__u64 word[2];
	struct {
		/* Word 0 */
		unsigned p : 1;			/**< Present. */
		unsigned : 1;
		unsigned ma : 3;		/**< Memory attribute. */
		unsigned a : 1;			/**< Accessed. */
		unsigned d : 1;			/**< Dirty. */
		unsigned pl : 2;		/**< Privilege level. */
		unsigned ar : 3;		/**< Access rights. */
		unsigned long long ppn : 38;	/**< Physical Page Number, a.k.a. PFN. */
		unsigned : 2;
		unsigned ed : 1;
		unsigned ig1 : 11;

		/* Word 1 */
		unsigned : 2;
		unsigned ps : 6;		/**< Page size will be 2^ps. */
		unsigned key : 24;		/**< Protection key, unused. */
		unsigned : 32;
	} __attribute__ ((packed));
} __attribute__ ((packed));
typedef union tlb_entry tlb_entry_t;

extern void tc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, bool dtc);
extern void dtc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry);
extern void itc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry);

extern void tr_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, bool dtr, index_t tr);
extern void dtr_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, index_t tr);
extern void itr_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, index_t tr);

extern void dtlb_kernel_mapping_insert(__address page, __address frame, bool dtr, index_t tr);

extern void dtc_pte_copy(pte_t *t);
extern void itc_pte_copy(pte_t *t);

extern void alternate_instruction_tlb_fault(__u64 vector, struct exception_regdump *pstate);
extern void alternate_data_tlb_fault(__u64 vector, struct exception_regdump *pstate);
extern void data_nested_tlb_fault(__u64 vector, struct exception_regdump *pstate);
extern void data_dirty_bit_fault(__u64 vector, struct exception_regdump *pstate);
extern void instruction_access_bit_fault(__u64 vector, struct exception_regdump *pstate);
extern void data_access_bit_fault(__u64 vector, struct exception_regdump *pstate);
extern void page_not_present(__u64 vector, struct exception_regdump *pstate);

#endif
