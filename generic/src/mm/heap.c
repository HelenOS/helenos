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

#include <mm/heap.h>
#include <synch/spinlock.h>
#include <func.h>
#include <memstr.h>
#include <panic.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <arch.h>
#include <align.h>

/*
 * First-fit algorithm.
 * Simple, but hopefully correct.
 * Chunks being freed are tested for mergability with their neighbours.
 */

static chunk_t *chunk0;
static spinlock_t heaplock;

void early_heap_init(__address heap, size_t size)
{
	spinlock_initialize(&heaplock, "heap_lock");
	memsetb(heap, size, 0);
	chunk0 = (chunk_t *) heap;
	chunk0->used = 0;
	chunk0->size = size - sizeof(chunk_t);
	chunk0->next = NULL;
	chunk0->prev = NULL;
}

/*
 * Uses first-fit algorithm.
 */
void *early_malloc(size_t size)
{
	ipl_t ipl;
	chunk_t *x, *y, *z;

	size = ALIGN_UP(size, sizeof(__native));

	if (size == 0)
		panic("zero-size allocation request");
		
	x = chunk0;
	ipl = interrupts_disable();
	spinlock_lock(&heaplock);		
	while (x) {
		if (x->used || x->size < size) {
			x = x->next;
			continue;
		}
		
		x->used = 1;
    
		/*
		 * If the chunk exactly matches required size or if truncating
		 * it would not provide enough space for storing a new chunk
		 * header plus at least one byte of data, we are finished.
		 */
		if (x->size < size + sizeof(chunk_t) + 1) {
			spinlock_unlock(&heaplock);
			interrupts_restore(ipl);
			return &x->data[0];
		}

		/*
		 * Truncate x and create a new chunk.
		 */
		y = (chunk_t *) (((__address) x) + size + sizeof(chunk_t));
		y->used = 0;
		y->size = x->size - size - sizeof(chunk_t);
		y->prev = x;
		y->next = NULL;
		
		if (z = x->next) {
			z->prev = y;
			y->next = z;
		}
		
		x->size = size;
		x->next = y;
		spinlock_unlock(&heaplock);
		interrupts_restore(ipl);

		return &x->data[0];
	}
	spinlock_unlock(&heaplock);
	interrupts_restore(ipl);
	return NULL;
}

void early_free(void *ptr)
{
	ipl_t ipl;
	chunk_t *x, *y, *z;

	if (!ptr)
		panic("free on NULL");


	y = (chunk_t *) (((__u8 *) ptr) - sizeof(chunk_t));
	if (y->used != 1)
		panic("freeing unused/damaged chunk");

	ipl = interrupts_disable();
	spinlock_lock(&heaplock);
	x = y->prev;
	z = y->next;
	/* merge x and y */
	if (x && !x->used) {
		x->size += y->size + sizeof(chunk_t);
		x->next = z;
		if (z)
			z->prev = x;
		y = x;
	}
	/* merge y and z or merge (x merged with y) and z */
	if (z && !z->used) {
		y->size += z->size + sizeof(chunk_t);
		y->next = z->next;
		if (z->next)  {
			/* y is either y or x */
			z->next->prev = y;
		}
	}
	y->used = 0;
	spinlock_unlock(&heaplock);
	interrupts_restore(ipl);
}
