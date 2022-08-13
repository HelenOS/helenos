/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */
/** @file
 */

#ifndef KERN_PAGE_H_
#define KERN_PAGE_H_

#include <typedefs.h>
#include <proc/task.h>
#include <mm/as.h>
#include <arch/mm/page.h>

#define P2SZ(pages) \
	((pages) << PAGE_WIDTH)

/** Operations to manipulate page mappings. */
typedef struct {
	void (*mapping_insert)(as_t *, uintptr_t, uintptr_t, unsigned int);
	void (*mapping_remove)(as_t *, uintptr_t);
	bool (*mapping_find)(as_t *, uintptr_t, bool, pte_t *);
	void (*mapping_update)(as_t *, uintptr_t, bool, pte_t *);
	void (*mapping_make_global)(uintptr_t, size_t);
} page_mapping_operations_t;

extern page_mapping_operations_t *page_mapping_operations;

extern void page_init(void);
extern void page_table_lock(as_t *, bool);
extern void page_table_unlock(as_t *, bool);
extern bool page_table_locked(as_t *);
extern void page_mapping_insert(as_t *, uintptr_t, uintptr_t, unsigned int);
extern void page_mapping_remove(as_t *, uintptr_t);
extern bool page_mapping_find(as_t *, uintptr_t, bool, pte_t *);
extern void page_mapping_update(as_t *, uintptr_t, bool, pte_t *);
extern void page_mapping_make_global(uintptr_t, size_t);
extern pte_t *page_table_create(unsigned int);
extern void page_table_destroy(pte_t *);

extern errno_t page_find_mapping(uintptr_t, uintptr_t *);
extern sys_errno_t sys_page_find_mapping(uintptr_t, uspace_ptr_uintptr_t);

#endif

/** @}
 */
