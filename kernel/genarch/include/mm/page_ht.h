/*
 * Copyright (C) 2006 Jakub Jermar
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

/** @addtogroup genarchmm
 * @{
 */
/**
 * @file
 * @brief	This is the generic page hash table interface.
 */

#ifdef CONFIG_PAGE_HT

#ifndef KERN_PAGE_HT_H_
#define KERN_PAGE_HT_H_

#include <mm/page.h>
#include <typedefs.h>
#include <arch/types.h>
#include <adt/list.h>
#include <adt/hash_table.h>

#define PAGE_HT_KEYS	2
#define KEY_AS		0
#define KEY_PAGE	1

#define PAGE_HT_ENTRIES_BITS	13
#define PAGE_HT_ENTRIES		(1<<PAGE_HT_ENTRIES_BITS)

#define PTE_VALID(pte)		((pte) != NULL)
#define PTE_PRESENT(pte)	((pte)->p != 0)
#define PTE_GET_FRAME(pte)	((pte)->frame)
#define PTE_READABLE(pte)	1
#define PTE_WRITABLE(pte)	((pte)->w != 0)
#define PTE_EXECUTABLE(pte)	((pte)->x != 0)

#define SET_PTL0_ADDRESS(x)

struct pte {
	link_t link;		/**< Page hash table link. */
	as_t *as;		/**< Address space. */
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
};

extern page_mapping_operations_t ht_mapping_operations;
extern mutex_t page_ht_lock;
extern hash_table_t page_ht;
extern hash_table_operations_t ht_operations;

#endif

#endif

/** @}
 */
