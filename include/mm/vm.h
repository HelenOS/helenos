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

#ifndef __VM_H__
#define __VM_H__

#include <arch/mm/page.h>
#include <arch/mm/vm.h>
#include <arch/mm/asid.h>
#include <arch/types.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <list.h>

#define KERNEL_ADDRESS_SPACE_START	KERNEL_ADDRESS_SPACE_START_ARCH
#define KERNEL_ADDRESS_SPACE_END	KERNEL_ADDRESS_SPACE_END_ARCH
#define USER_ADDRESS_SPACE_START	USER_ADDRESS_SPACE_START_ARCH
#define USER_ADDRESS_SPACE_END		USER_ADDRESS_SPACE_END_ARCH

#define IS_KA(addr)	((addr)>=KERNEL_ADDRESS_SPACE_START && (addr)<=KERNEL_ADDRESS_SPACE_END)

#define UTEXT_ADDRESS	UTEXT_ADDRESS_ARCH
#define USTACK_ADDRESS	USTACK_ADDRESS_ARCH
#define UDATA_ADDRESS	UDATA_ADDRESS_ARCH

enum vm_type {
	VMA_TEXT = 1, VMA_DATA, VMA_STACK 
};

/*
 * Each vm_area_t structure describes one continuous area of virtual memory.
 * In the future, it should not be difficult to support shared areas of vm.
 */
struct vm_area {
	spinlock_t lock;
	link_t link;
	vm_type_t type;
	int size;
	__address address;
	__address *mapping;
};

/*
 * vm_t contains the list of vm_areas of userspace accessible
 * pages for one or more tasks. Ranges of kernel memory pages are not
 * supposed to figure in the list as they are shared by all tasks and
 * set up during system initialization.
 */
struct vm {
	spinlock_t lock;
	link_t vm_area_head;
	pte_t *ptl0;
	asid_t asid;
};

extern vm_t * vm_create(pte_t *ptl0);
extern void vm_destroy(vm_t *m);

extern vm_area_t *vm_area_create(vm_t *m, vm_type_t type, size_t size, __address addr);
extern void vm_area_destroy(vm_area_t *a);

extern void vm_area_map(vm_area_t *a, vm_t *m);
extern void vm_area_unmap(vm_area_t *a, vm_t *m);

extern void vm_install(vm_t *m);
extern void vm_uninstall(vm_t *m);

#endif
