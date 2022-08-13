/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch_mm
 * @{
 */
/**
 * @file
 * @brief This is the generic page hash table interface.
 */

#ifdef CONFIG_PAGE_HT

#ifndef KERN_PAGE_HT_H_
#define KERN_PAGE_HT_H_

#include <typedefs.h>
#include <mm/as.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <adt/hash_table.h>

#define KEY_AS        0
#define KEY_PAGE      1

/* Macros for querying page hash table PTEs. */
#define PTE_VALID(pte)       ((void *) (pte) != NULL)
#define PTE_PRESENT(pte)     ((pte)->p != 0)
#define PTE_GET_FRAME(pte)   ((pte)->frame)
#define PTE_READABLE(pte)    1
#define PTE_WRITABLE(pte)    ((pte)->w != 0)
#define PTE_EXECUTABLE(pte)  ((pte)->x != 0)

extern as_operations_t as_ht_operations;
extern page_mapping_operations_t ht_mapping_operations;

extern slab_cache_t *pte_cache;
extern hash_table_t page_ht;
extern hash_table_ops_t ht_ops;

#endif

#endif

/** @}
 */
