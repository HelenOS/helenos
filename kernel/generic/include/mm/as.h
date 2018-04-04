/*
 * Copyright (c) 2010 Jakub Jermar
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

#ifndef KERN_AS_H_
#define KERN_AS_H_

#include <typedefs.h>
#include <abi/mm/as.h>
#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <arch/mm/asid.h>
#include <arch/istate.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <adt/list.h>
#include <adt/btree.h>
#include <lib/elf.h>
#include <arch.h>

#define AS                   THE->as


/**
 * Defined to be true if user address space and kernel address space shadow each
 * other.
 *
 */
#define KERNEL_ADDRESS_SPACE_SHADOWED  KERNEL_ADDRESS_SPACE_SHADOWED_ARCH

#define KERNEL_ADDRESS_SPACE_START  KERNEL_ADDRESS_SPACE_START_ARCH
#define KERNEL_ADDRESS_SPACE_END    KERNEL_ADDRESS_SPACE_END_ARCH
#define USER_ADDRESS_SPACE_START    USER_ADDRESS_SPACE_START_ARCH
#define USER_ADDRESS_SPACE_END      USER_ADDRESS_SPACE_END_ARCH

/** Kernel address space. */
#define FLAG_AS_KERNEL  (1 << 0)

/* Address space area attributes. */
#define AS_AREA_ATTR_NONE     0
#define AS_AREA_ATTR_PARTIAL  1  /**< Not fully initialized area. */

/** The page fault was resolved by as_page_fault(). */
#define AS_PF_OK     0

/** The page fault was caused by memcpy_from_uspace() or memcpy_to_uspace(). */
#define AS_PF_DEFER  1

/** The page fault was not resolved by as_page_fault(). */
#define AS_PF_FAULT  2

/** The page fault was not resolved by as_page_fault(). Non-verbose version. */
#define AS_PF_SILENT 3

/** Address space structure.
 *
 * as_t contains the list of as_areas of userspace accessible
 * pages for one or more tasks. Ranges of kernel memory pages are not
 * supposed to figure in the list as they are shared by all tasks and
 * set up during system initialization.
 *
 */
typedef struct as {
	/** Protected by asidlock. */
	link_t inactive_as_with_asid_link;

	/**
	 * Number of processors on which this
	 * address space is active. Protected by
	 * asidlock.
	 */
	size_t cpu_refcount;

	/** Address space identifier.
	 *
	 * Constant on architectures that do not
	 * support ASIDs. Protected by asidlock.
	 *
	 */
	asid_t asid;

	/** Number of references (i.e. tasks that reference this as). */
	atomic_t refcount;

	mutex_t lock;

	/** B+tree of address space areas. */
	btree_t as_area_btree;

	/** Non-generic content. */
	as_genarch_t genarch;

	/** Architecture specific content. */
	as_arch_t arch;
} as_t;

typedef struct {
	pte_t *(*page_table_create)(unsigned int);
	void (*page_table_destroy)(pte_t *);
	void (*page_table_lock)(as_t *, bool);
	void (*page_table_unlock)(as_t *, bool);
	bool (*page_table_locked)(as_t *);
} as_operations_t;

/**
 * This structure contains information associated with the shared address space
 * area.
 *
 */
typedef struct {
	/** This lock must be acquired only when the as_area lock is held. */
	mutex_t lock;
	/** This structure can be deallocated if refcount drops to 0. */
	size_t refcount;
	/** True if the area has been ever shared. */
	bool shared;

	/**
	 * B+tree containing complete map of anonymous pages of the shared area.
	 */
	btree_t pagemap;

	/** Address space area backend. */
	struct mem_backend *backend;
	/** Address space area shared data. */
	void *backend_shared_data;
} share_info_t;

/** Page fault access type. */
typedef enum {
	PF_ACCESS_READ,
	PF_ACCESS_WRITE,
	PF_ACCESS_EXEC,
	PF_ACCESS_UNKNOWN
} pf_access_t;

struct mem_backend;

/** Backend data stored in address space area. */
typedef union mem_backend_data {
	/* anon_backend members */
	struct {
	};

	/** elf_backend members */
	struct {
		elf_header_t *elf;
		elf_segment_header_t *segment;
	};

	/** phys_backend members */
	struct {
		uintptr_t base;
		size_t frames;
		bool anonymous;
	};

	/** user_backend members */
	struct {
		as_area_pager_info_t pager_info;
	};

} mem_backend_data_t;

/** Address space area structure.
 *
 * Each as_area_t structure describes one contiguous area of virtual memory.
 *
 */
typedef struct {
	mutex_t lock;

	/** Containing address space. */
	as_t *as;

	/** Memory flags. */
	unsigned int flags;

	/** Address space area attributes. */
	unsigned int attributes;

	/** Number of pages in the area. */
	size_t pages;

	/** Number of resident pages in the area. */
	size_t resident;

	/** Base address of this area. */
	uintptr_t base;

	/** Map of used space. */
	btree_t used_space;

	/**
	 * If the address space area is shared. this is
	 * a reference to the share info structure.
	 */
	share_info_t *sh_info;

	/** Memory backend backing this address space area. */
	struct mem_backend *backend;

	/** Data to be used by the backend. */
	mem_backend_data_t backend_data;
} as_area_t;

/** Address space area backend structure. */
typedef struct mem_backend {
	bool (*create)(as_area_t *);
	bool (*resize)(as_area_t *, size_t);
	void (*share)(as_area_t *);
	void (*destroy)(as_area_t *);

	bool (*is_resizable)(as_area_t *);
	bool (*is_shareable)(as_area_t *);

	int (*page_fault)(as_area_t *, uintptr_t, pf_access_t);
	void (*frame_free)(as_area_t *, uintptr_t, uintptr_t);

	bool (*create_shared_data)(as_area_t *);
	void (*destroy_shared_data)(void *);
} mem_backend_t;

extern as_t *AS_KERNEL;

extern as_operations_t *as_operations;
extern list_t inactive_as_with_asid_list;

extern void as_init(void);

extern as_t *as_create(unsigned int);
extern void as_destroy(as_t *);
extern void as_hold(as_t *);
extern void as_release(as_t *);
extern void as_switch(as_t *, as_t *);
extern int as_page_fault(uintptr_t, pf_access_t, istate_t *);

extern as_area_t *as_area_create(as_t *, unsigned int, size_t, unsigned int,
    mem_backend_t *, mem_backend_data_t *, uintptr_t *, uintptr_t);
extern errno_t as_area_destroy(as_t *, uintptr_t);
extern errno_t as_area_resize(as_t *, uintptr_t, size_t, unsigned int);
extern errno_t as_area_share(as_t *, uintptr_t, size_t, as_t *, unsigned int,
    uintptr_t *, uintptr_t);
extern errno_t as_area_change_flags(as_t *, unsigned int, uintptr_t);

extern unsigned int as_area_get_flags(as_area_t *);
extern bool as_area_check_access(as_area_t *, pf_access_t);
extern size_t as_area_get_size(uintptr_t);
extern bool used_space_insert(as_area_t *, uintptr_t, size_t);
extern bool used_space_remove(as_area_t *, uintptr_t, size_t);

/* Interface to be implemented by architectures. */

#ifndef as_constructor_arch
extern errno_t as_constructor_arch(as_t *, unsigned int);
#endif /* !def as_constructor_arch */

#ifndef as_destructor_arch
extern int as_destructor_arch(as_t *);
#endif /* !def as_destructor_arch */

#ifndef as_create_arch
extern errno_t as_create_arch(as_t *, unsigned int);
#endif /* !def as_create_arch */

#ifndef as_install_arch
extern void as_install_arch(as_t *);
#endif /* !def as_install_arch */

#ifndef as_deinstall_arch
extern void as_deinstall_arch(as_t *);
#endif /* !def as_deinstall_arch */

/* Backend declarations and functions. */
extern mem_backend_t anon_backend;
extern mem_backend_t elf_backend;
extern mem_backend_t phys_backend;
extern mem_backend_t user_backend;

/* Address space area related syscalls. */
extern sysarg_t sys_as_area_create(uintptr_t, size_t, unsigned int, uintptr_t,
    as_area_pager_info_t *);
extern sys_errno_t sys_as_area_resize(uintptr_t, size_t, unsigned int);
extern sys_errno_t sys_as_area_change_flags(uintptr_t, unsigned int);
extern sys_errno_t sys_as_area_destroy(uintptr_t);

/* Introspection functions. */
extern void as_get_area_info(as_t *, as_area_info_t **, size_t *);
extern void as_print(as_t *);

#endif

/** @}
 */
