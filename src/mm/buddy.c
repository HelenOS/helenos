/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <mm/buddy.h>
#include <mm/frame.h>
#include <mm/heap.h>
#include <arch/types.h>
#include <typedefs.h>
#include <list.h>
#include <debug.h>

/** Create buddy system
 *
 * Allocate memory for and initialize new buddy system.
 *
 * @param max_order The biggest allocable size will be 2^max_order.
 * @param op Operations for new buddy system.
 *
 * @return New buddy system.
 */
buddy_system_t *buddy_system_create(__u8 max_order, buddy_operations_t *op)
{
	buddy_system_t *b;
	int i;

	ASSERT(max_order < BUDDY_SYSTEM_INNER_BLOCK);

	ASSERT(op->find_buddy);
	ASSERT(op->set_order);
	ASSERT(op->get_order);
	ASSERT(op->bisect);
	ASSERT(op->coalesce);

	/*
	 * Allocate memory for structure describing the whole buddy system.
	 */	
	b = (buddy_system_t *) early_malloc(sizeof(buddy_system_t));
	
	if (b) {
		/*
		 * Allocate memory for all orders this buddy system will work with.
		 */
		b->order = (link_t *) early_malloc(max_order * sizeof(link_t));
		if (!b->order) {
			early_free(b);
			return NULL;
		}
	
		for (i = 0; i < max_order; i++)
			list_initialize(&b->order[i]);
	
		b->max_order = max_order;
		b->op = op;
	}
	
	return b;
}

/** Allocate block from buddy system.
 *
 * Allocate block from buddy system.
 *
 * @param b Buddy system pointer.
 * @param i Returned block will be 2^i big.
 *
 * @return Block of data represented by link_t.
 */
link_t *buddy_system_alloc(buddy_system_t *b, __u8 i)
{
	link_t *res, *hlp;

	ASSERT(i < b->max_order);

	/*
	 * If the list of order i is not empty,
	 * the request can be immediatelly satisfied.
	 */
	if (!list_empty(&b->order[i])) {
		res = b->order[i].next;
		list_remove(res);
		return res;
	}
	
	/*
	 * If order i is already the maximal order,
	 * the request cannot be satisfied.
	 */
	if (i == b->max_order - 1)
		return NULL;

	/*
	 * Try to recursively satisfy the request from higher order lists.
	 */	
	hlp = buddy_system_alloc(b, i + 1);
	
	/*
	 * The request could not be satisfied
	 * from higher order lists.
	 */
	if (!hlp)
		return NULL;
		
	res = hlp;
	
	/*
	 * Bisect the block and set order of both of its parts to i.
	 */
	hlp = b->op->bisect(res);
	b->op->set_order(res, i);
	b->op->set_order(hlp, i);
	
	/*
	 * Return the other half to buddy system.
	 */
	buddy_system_free(b, hlp);
	
	return res;
	
}

/** Return block to buddy system.
 *
 * Return block to buddy system.
 *
 * @param b Buddy system pointer.
 * @param block Block to return.
 */
void buddy_system_free(buddy_system_t *b, link_t *block)
{
	link_t *buddy, *hlp;
	__u8 i;
	
	/*
	 * Determine block's order.
	 */
	i = b->op->get_order(block);

	ASSERT(i < b->max_order);

	/*
	 * See if there is any buddy in the list of order i.
	 */
	buddy = b->op->find_buddy(block);
	
	if (buddy && i != b->max_order - 1) {

		ASSERT(b->op->get_order(buddy) == i);
		
		/*
		 * Remove buddy from the list of order i.
		 */
		list_remove(buddy);
		
		/*
		 * Invalidate order of both block and buddy.
		 */
		b->op->set_order(block, BUDDY_SYSTEM_INNER_BLOCK);
		b->op->set_order(buddy, BUDDY_SYSTEM_INNER_BLOCK);
		
		/*
		 * Coalesce block and buddy into one block.
		 */
		hlp = b->op->coalesce(block, buddy);

		/*
		 * Set order of the coalesced block to i + 1.
		 */
		b->op->set_order(hlp, i + 1);

		/*
		 * Recursively add the coalesced block to the list of order i + 1.
		 */
		buddy_system_free(b, hlp);
	}
	else {
		/*
		 * Insert block into the list of order i.
		 */
		list_append(block, &b->order[i]);
	}

}
