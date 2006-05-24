/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __PAGE_H__
#define __PAGE_H__

#include <arch/mm/asid.h>
#include <arch/types.h>
#include <typedefs.h>
#include <memstr.h>

#define PAGE_CACHEABLE_SHIFT		0
#define PAGE_NOT_CACHEABLE_SHIFT	PAGE_CACHEABLE_SHIFT
#define PAGE_PRESENT_SHIFT		1
#define PAGE_NOT_PRESENT_SHIFT		PAGE_PRESENT_SHIFT
#define PAGE_USER_SHIFT			2
#define PAGE_KERNEL_SHIFT		PAGE_USER_SHIFT
#define PAGE_READ_SHIFT			3
#define PAGE_WRITE_SHIFT		4
#define PAGE_EXEC_SHIFT			5
#define PAGE_GLOBAL_SHIFT		6

#define PAGE_NOT_CACHEABLE	(0<<PAGE_CACHEABLE_SHIFT)
#define PAGE_CACHEABLE		(1<<PAGE_CACHEABLE_SHIFT)

#define PAGE_PRESENT		(0<<PAGE_PRESENT_SHIFT)
#define PAGE_NOT_PRESENT	(1<<PAGE_PRESENT_SHIFT)

#define PAGE_USER		(1<<PAGE_USER_SHIFT)
#define PAGE_KERNEL		(0<<PAGE_USER_SHIFT)

#define PAGE_READ		(1<<PAGE_READ_SHIFT)
#define PAGE_WRITE		(1<<PAGE_WRITE_SHIFT)
#define PAGE_EXEC		(1<<PAGE_EXEC_SHIFT)

#define PAGE_GLOBAL		(1<<PAGE_GLOBAL_SHIFT)

/** Page fault access type. */
enum pf_access {
	PF_ACCESS_READ,
	PF_ACCESS_WRITE,
	PF_ACCESS_EXEC
};
typedef enum pf_access pf_access_t;

/** Operations to manipulate page mappings. */
struct page_mapping_operations {
	void (* mapping_insert)(as_t *as, __address page, __address frame, int flags);
	void (* mapping_remove)(as_t *as, __address page);
	pte_t *(* mapping_find)(as_t *as, __address page);
};
typedef struct page_mapping_operations page_mapping_operations_t;

extern page_mapping_operations_t *page_mapping_operations;

extern void page_init(void);
extern void page_table_lock(as_t *as, bool lock);
extern void page_table_unlock(as_t *as, bool unlock);
extern void page_mapping_insert(as_t *as, __address page, __address frame, int flags);
extern void page_mapping_remove(as_t *as, __address page);
extern pte_t *page_mapping_find(as_t *as, __address page);
extern pte_t *page_table_create(int flags);
extern void map_structure(__address s, size_t size);
extern __address hw_map(__address physaddr, size_t size);

#endif
