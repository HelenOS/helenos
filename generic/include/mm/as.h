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

#ifndef __AS_H__
#define __AS_H__

#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <arch/mm/asid.h>
#include <arch/types.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <adt/list.h>

#define KERNEL_ADDRESS_SPACE_START	KERNEL_ADDRESS_SPACE_START_ARCH
#define KERNEL_ADDRESS_SPACE_END	KERNEL_ADDRESS_SPACE_END_ARCH
#define USER_ADDRESS_SPACE_START	USER_ADDRESS_SPACE_START_ARCH
#define USER_ADDRESS_SPACE_END		USER_ADDRESS_SPACE_END_ARCH

#define IS_KA(addr)	((addr)>=KERNEL_ADDRESS_SPACE_START && (addr)<=KERNEL_ADDRESS_SPACE_END)

#define USTACK_ADDRESS	USTACK_ADDRESS_ARCH

#define FLAG_AS_KERNEL	    (1 << 0)	/**< Kernel address space. */

enum as_area_type {
	AS_AREA_TEXT = 1, AS_AREA_DATA, AS_AREA_STACK 
};

/** Address space area structure.
 *
 * Each as_area_t structure describes one contiguous area of virtual memory.
 * In the future, it should not be difficult to support shared areas.
 */
struct as_area {
	SPINLOCK_DECLARE(lock);
	link_t link;
	as_area_type_t type;
	size_t size;		/**< Size of this area in multiples of PAGE_SIZE. */
	__address base;		/**< Base address of this area. */
};

/** Address space structure.
 *
 * as_t contains the list of as_areas of userspace accessible
 * pages for one or more tasks. Ranges of kernel memory pages are not
 * supposed to figure in the list as they are shared by all tasks and
 * set up during system initialization.
 */
struct as {
	/** Protected by asidlock. Must be acquired before as->lock. */
	link_t inactive_as_with_asid_link;

	SPINLOCK_DECLARE(lock);

	/** Number of processors on wich is this address space active. */
	count_t refcount;

	link_t as_area_head;

	/** Page table pointer. Constant on architectures that use global page hash table. */
	pte_t *page_table;

	/** Address space identifier. Constant on architectures that do not support ASIDs.*/
	asid_t asid;
};

struct as_operations {
	pte_t *(* page_table_create)(int flags);
};
typedef struct as_operations as_operations_t;

extern as_t *AS_KERNEL;
extern as_operations_t *as_operations;

extern spinlock_t as_lock;
extern link_t inactive_as_with_asid_head;

extern void as_init(void);
extern as_t *as_create(int flags);
extern as_area_t *as_area_create(as_t *as, as_area_type_t type, size_t size, __address base);
extern void as_set_mapping(as_t *as, __address page, __address frame);
extern int as_page_fault(__address page);
extern void as_switch(as_t *old, as_t *new);

/* Interface to be implemented by architectures. */
#ifndef as_install_arch
extern void as_install_arch(as_t *as);
#endif /* !def as_install_arch */

#endif
