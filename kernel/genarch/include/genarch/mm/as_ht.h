/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch_mm
 * @{
 */
/** @file
 */

#ifndef KERN_AS_HT_H_
#define KERN_AS_HT_H_

#include <mm/mm.h>
#include <adt/hash_table.h>
#include <typedefs.h>

typedef struct {
} as_genarch_t;

struct as;

typedef struct pte {
	ht_link_t link;		/**< Page hash table link. */
	struct as *as;		/**< Address space. */
	uintptr_t page;		/**< Virtual memory page. */
	uintptr_t frame;	/**< Physical memory frame. */
	unsigned g : 1;		/**< Global page. */
	unsigned x : 1;		/**< Execute. */
	unsigned w : 1;		/**< Writable. */
	unsigned k : 1;		/**< Kernel privileges required. */
	unsigned c : 1;		/**< Cacheable. */
	unsigned a : 1;		/**< Accessed. */
	unsigned d : 1;		/**< Dirty. */
	unsigned p : 1;		/**< Present. */
} pte_t;

#endif

/** @}
 */
