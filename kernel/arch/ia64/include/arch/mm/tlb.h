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

/** @addtogroup ia64mm
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_TLB_H_
#define KERN_ia64_TLB_H_

#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <arch/interrupt.h>
#include <typedefs.h>

/** Data and instruction Translation Register indices. */
#define DTR_KERNEL   0
#define ITR_KERNEL   0
#define DTR_KSTACK1  4
#define DTR_KSTACK2  5

/** Portion of TLB insertion format data structure. */
typedef union {
	uint64_t word[2];
	struct {
		/* Word 0 */
		unsigned int p : 1;           /**< Present. */
		unsigned int : 1;
		unsigned int ma : 3;          /**< Memory attribute. */
		unsigned int a : 1;           /**< Accessed. */
		unsigned int d : 1;           /**< Dirty. */
		unsigned int pl : 2;          /**< Privilege level. */
		unsigned int ar : 3;          /**< Access rights. */
		unsigned long long ppn : 38;  /**< Physical Page Number, a.k.a. PFN. */
		unsigned int : 2;
		unsigned int ed : 1;
		unsigned int ig1 : 11;

		/* Word 1 */
		unsigned int : 2;
		unsigned int ps : 6;    /**< Page size will be 2^ps. */
		unsigned int key : 24;  /**< Protection key, unused. */
		unsigned int : 32;
	} __attribute__ ((packed));
} __attribute__ ((packed)) tlb_entry_t;

extern void tc_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, bool dtc);
extern void dtc_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry);
extern void itc_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry);

extern void tr_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, bool dtr, size_t tr);
extern void dtr_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, size_t tr);
extern void itr_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, size_t tr);

extern void dtlb_kernel_mapping_insert(uintptr_t page, uintptr_t frame, bool dtr, size_t tr);
extern void dtr_purge(uintptr_t page, size_t width);

extern void dtc_pte_copy(pte_t *t);
extern void itc_pte_copy(pte_t *t);

extern void alternate_instruction_tlb_fault(unsigned int, istate_t *);
extern void alternate_data_tlb_fault(unsigned int, istate_t *);
extern void data_nested_tlb_fault(unsigned int, istate_t *);
extern void data_dirty_bit_fault(unsigned int, istate_t *);
extern void instruction_access_bit_fault(unsigned int, istate_t *);
extern void data_access_bit_fault(unsigned int, istate_t *);
extern void data_access_rights_fault(unsigned int, istate_t *);
extern void page_not_present(unsigned int, istate_t *);

#endif

/** @}
 */
