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
#include <print.h>

/** Create buddy system
 *
 * Allocate memory for and initialize new buddy system.
 *
 * @param max_order The biggest allocable size will be 2^max_order.
 * @param op Operations for new buddy system.
 * @param data Pointer to be used by implementation.
 *
 * @return New buddy system.
 */
buddy_system_t *buddy_system_create(__u8 max_order, buddy_system_operations_t *op, void *data)
{
	buddy_system_t *b;
	int i;

	ASSERT(max_order < BUDDY_SYSTEM_INNER_BLOCK);

	ASSERT(op->find_buddy);
	ASSERT(op->set_order);
	ASSERT(op->get_order);
	ASSERT(op->bisect);
	ASSERT(op->coalesce);
	ASSERT(op->mark_busy);

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
		b->data = data;
	}
	
	return b;
}

/** Check if buddy system can allocate block
 *
 * @param b Buddy system pointer
 * @param i Size of the block (2^i)
 *
 * @return True if block can be allocated
 */
bool buddy_system_can_alloc(buddy_system_t *b, __u8 i) {
	__u8 k;
	
	ASSERT(i < b->max_order);
	
	for (k=i; k < b->max_order; k++) {
		if (!list_empty(&b->order[k])) {
			return true;
		}
	}
	
	return false;
	
}

/** Allocate block from buddy system.
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
		b->op->mark_busy(b, res);
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
	hlp = b->op->bisect(b, res);
	b->op->set_order(b, res, i);
	b->op->set_order(b, hlp, i);
	
	/*
	 * Return the other half to buddy system.
	 * PROBLEM!!!! FILL FIND OTHER PART AS BUDDY AND LINK TOGETHER
	 */
	b->op->mark_busy(b, res);
	buddy_system_free(b, hlp);
	
	return res;
	
}

/** Return block to buddy system.
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
	i = b->op->get_order(b, block);

	ASSERT(i < b->max_order);

	if (i != b->max_order - 1) {
		/*
		 * See if there is any buddy in the list of order i.
		 */
		buddy = b->op->find_buddy(b, block);
		if (buddy) {

			ASSERT(b->op->get_order(b, buddy) == i);
			/*
			 * Remove buddy from the list of order i.
			 */
			list_remove(buddy);
		
			/*
			 * Invalidate order of both block and buddy.
			 */
			b->op->set_order(b, block, BUDDY_SYSTEM_INNER_BLOCK);
			b->op->set_order(b, buddy, BUDDY_SYSTEM_INNER_BLOCK);
		
			/*
			 * Coalesce block and buddy into one block.
			 */
			hlp = b->op->coalesce(b, block, buddy);

			/*
			 * Set order of the coalesced block to i + 1.
			 */
			b->op->set_order(b, hlp, i + 1);

			/*
			 * Recursively add the coalesced block to the list of order i + 1.
			 */
			buddy_system_free(b, hlp);
			return;
		}
	}

	/*
	 * Insert block into the list of order i.
	 */
	list_append(block, &b->order[i]);

}



/** Prints out structure of buddy system
 *
 * @param b Pointer to buddy system
 * @param es Element size
 */
void buddy_system_structure_print(buddy_system_t *b, size_t elem_size) {
	index_t i;
	count_t cnt, elem_count = 0, block_count = 0;
	link_t * cur;
	

	printf("Order\tBlocks\tSize    \tBlock size\tElems per block\n");
	printf("-----\t------\t--------\t----------\t---------------\n");
	
	for (i=0;i < b->max_order; i++) {
		cnt = 0;
		if (!list_empty(&b->order[i])) {
			for (cur = b->order[i].next; cur != &b->order[i]; cur = cur->next) cnt++;
		}
	
		printf("#%d\t%d\t%d kb\t\t%d kb\t\t%d\n", i, cnt, (cnt * (1 << i) * elem_size) >> 10, ((1 << i) * elem_size) >> 10, 1 << i);
		
		block_count += cnt;
		elem_count += (1 << i) * cnt;
	}
	printf("-----\t------\t--------\t----------\t---------------\n");
	printf("Buddy system contains %d elements (%d blocks)\n" , elem_count, block_count);

}
