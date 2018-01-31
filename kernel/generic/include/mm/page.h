/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup genericmm
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
extern sys_errno_t sys_page_find_mapping(uintptr_t, uintptr_t *);

#endif

/** @}
 */
