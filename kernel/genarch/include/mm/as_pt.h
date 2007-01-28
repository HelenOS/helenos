/*
 * Copyright (c) 2006 Jakub Jermar
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
/** @file
 */

#ifndef KERN_AS_PT_H_
#define KERN_AS_PT_H_

#include <mm/mm.h>
#include <arch/mm/asid.h>
#include <adt/list.h>
#include <adt/btree.h>
#include <synch/mutex.h>

#define AS_PAGE_TABLE

/** Address space structure.
 *
 * as_t contains the list of as_areas of userspace accessible
 * pages for one or more tasks. Ranges of kernel memory pages are not
 * supposed to figure in the list as they are shared by all tasks and
 * set up during system initialization.
 */
typedef struct {
	/** Protected by asidlock. */
	link_t inactive_as_with_asid_link;

	mutex_t lock;

	/** Number of references (i.e tasks that reference this as). */
	count_t refcount;

	/** Number of processors on wich is this address space active. */
	count_t cpu_refcount;

	/** B+tree of address space areas. */
	btree_t as_area_btree;
	
	/** Page table pointer. */
	pte_t *page_table;

	/** Address space identifier. Constant on architectures that do not support ASIDs.*/
	asid_t asid;
	
	/** Architecture specific content. */
	as_arch_t arch;
} as_t;

#endif

/** @}
 */
