/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_mm
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
	} __attribute__((packed));
} __attribute__((packed)) tlb_entry_t;

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
