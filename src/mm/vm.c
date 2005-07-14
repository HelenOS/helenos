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

#include <mm/vm.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/tlb.h>
#include <mm/heap.h>
#include <arch/mm/page.h>
#include <arch/types.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <config.h>
#include <list.h>
#include <panic.h>
#include <arch/asm.h>

vm_t *vm_create(void)
{
	vm_t *m;
	
	m = (vm_t *) malloc(sizeof(vm_t));
	if (m) {
		spinlock_initialize(&m->lock);
		list_initialize(&m->vm_area_head);
		m->ptl0 = NULL;
	}
	
	return m;
}

void vm_destroy(vm_t *m)
{
}

vm_area_t *vm_area_create(vm_t *m, vm_type_t type, size_t size, __address addr)
{
	pri_t pri;
	vm_area_t *a;
	
	if (addr % PAGE_SIZE)
		panic("addr not aligned to a page boundary");
	
	pri = cpu_priority_high();
	spinlock_lock(&m->lock);
	
	/*
	 * TODO: test vm_area which is to be created doesn't overlap with an existing one.
	 */
	
	a = (vm_area_t *) malloc(sizeof(vm_area_t));
	if (a) {
		int i;
	
		a->mapping = (__address *) malloc(size * sizeof(__address));
		if (!a->mapping) {
			free(a);
			spinlock_unlock(&m->lock);
			cpu_priority_restore(pri);
			return NULL;
		}
		
		for (i=0; i<size; i++)
			a->mapping[i] = frame_alloc(0);
		
		spinlock_initialize(&a->lock);
			
		link_initialize(&a->link);			
		a->type = type;
		a->size = size;
		a->address = addr;
		
		list_append(&a->link, &m->vm_area_head);

	}

	spinlock_unlock(&m->lock);
	cpu_priority_restore(pri);
	
	return a;
}

void vm_area_destroy(vm_area_t *a)
{
}

void vm_area_map(vm_area_t *a)
{
	int i, flags;
	pri_t pri;
	
	pri = cpu_priority_high();
	spinlock_lock(&a->lock);

	switch (a->type) {
		case VMA_TEXT:
			flags = PAGE_EXEC | PAGE_READ | PAGE_USER | PAGE_PRESENT | PAGE_CACHEABLE;
			break;
		case VMA_DATA:
		case VMA_STACK:
			flags = PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_PRESENT | PAGE_CACHEABLE;
			break;
		default:
			panic("unexpected vm_type_t %d", a->type); 
	}
	
	for (i=0; i<a->size; i++)
		map_page_to_frame(a->address + i*PAGE_SIZE, a->mapping[i], flags, 0);
		
	spinlock_unlock(&a->lock);
	cpu_priority_restore(pri);
}

void vm_area_unmap(vm_area_t *a)
{
	int i;
	pri_t pri;
	
	pri = cpu_priority_high();
	spinlock_lock(&a->lock);

	for (i=0; i<a->size; i++)		
		map_page_to_frame(a->address + i*PAGE_SIZE, 0, PAGE_NOT_PRESENT, 0);
	
	spinlock_unlock(&a->lock);
	cpu_priority_restore(pri);
}

void vm_install(vm_t *m)
{
	link_t *l;
	pri_t pri;
	
	pri = cpu_priority_high();
	spinlock_lock(&m->lock);

	for(l = m->vm_area_head.next; l != &m->vm_area_head; l = l->next)
		vm_area_map(list_get_instance(l, vm_area_t, link));

	spinlock_unlock(&m->lock);
	cpu_priority_restore(pri);
}

void vm_uninstall(vm_t *m)
{
	link_t *l;
	pri_t pri;
	
	pri = cpu_priority_high();

	tlb_shootdown_start();

	spinlock_lock(&m->lock);

	for(l = m->vm_area_head.next; l != &m->vm_area_head; l = l->next)
		vm_area_unmap(list_get_instance(l, vm_area_t, link));

	spinlock_unlock(&m->lock);

	tlb_shootdown_finalize();

	cpu_priority_restore(pri);
}
