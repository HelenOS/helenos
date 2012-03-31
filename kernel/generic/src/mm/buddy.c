/*
 * Copyright (c) 2005 Jakub Jermar
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

/**
 * @file
 * @brief	Buddy allocator framework.
 *
 * This file contains buddy system allocator framework.
 * Specialized functions are needed for this abstract framework to be useful.
 */

#include <mm/buddy.h>
#include <mm/frame.h>
#include <typedefs.h>
#include <debug.h>
#include <print.h>
#include <macros.h>

/** Return size needed for the buddy configuration data. */
size_t buddy_conf_size(size_t max_order)
{
	return sizeof(buddy_system_t) + (max_order + 1) * sizeof(link_t);
}


/** Create buddy system.
 *
 * Allocate memory for and initialize new buddy system.
 *
 * @param b		Preallocated buddy system control data.
 * @param max_order	The biggest allocable size will be 2^max_order.
 * @param op		Operations for new buddy system.
 * @param data		Pointer to be used by implementation.
 *
 * @return		New buddy system.
 */
void
buddy_system_create(buddy_system_t *b, uint8_t max_order,
    buddy_system_operations_t *op, void *data)
{
	int i;

	ASSERT(max_order < BUDDY_SYSTEM_INNER_BLOCK);

	ASSERT(op->find_buddy);
	ASSERT(op->set_order);
	ASSERT(op->get_order);
	ASSERT(op->bisect);
	ASSERT(op->coalesce);
	ASSERT(op->mark_busy);

	/*
	 * Use memory after our own structure.
	 */
	b->order = (list_t *) (&b[1]);
	
	for (i = 0; i <= max_order; i++)
		list_initialize(&b->order[i]);

	b->max_order = max_order;
	b->op = op;
	b->data = data;
}

/** Check if buddy system can allocate block.
 *
 * @param b		Buddy system pointer.
 * @param i		Size of the block (2^i).
 *
 * @return		True if block can be allocated.
 */
bool buddy_system_can_alloc(buddy_system_t *b, uint8_t i)
{
	uint8_t k;
	
	/*
	 * If requested block is greater then maximal block
	 * we know immediatly that we cannot satisfy the request.
	 */
	if (i > b->max_order)
		return false;

	/*
	 * Check if any bigger or equal order has free elements
	 */
	for (k = i; k <= b->max_order; k++) {
		if (!list_empty(&b->order[k])) {
			return true;
		}
	}
	
	return false;
}

/** Allocate PARTICULAR block from buddy system.
 *
 * @return		Block of data or NULL if no such block was found.
 */
link_t *buddy_system_alloc_block(buddy_system_t *b, link_t *block)
{
	link_t *left,*right, *tmp;
	uint8_t order;

	left = b->op->find_block(b, block, BUDDY_SYSTEM_INNER_BLOCK);
	ASSERT(left);
	list_remove(left);
	while (1) {
		if (!b->op->get_order(b, left)) {
			b->op->mark_busy(b, left);
			return left;
		}
		
		order = b->op->get_order(b, left);

		right = b->op->bisect(b, left);
		b->op->set_order(b, left, order - 1);
		b->op->set_order(b, right, order - 1);

		tmp = b->op->find_block(b, block, BUDDY_SYSTEM_INNER_BLOCK);

		if (tmp == right) {
			right = left;
			left = tmp;
		} 
		ASSERT(tmp == left);
		b->op->mark_busy(b, left);
		buddy_system_free(b, right);
		b->op->mark_available(b, left);
	}
}

/** Allocate block from buddy system.
 *
 * @param b		Buddy system pointer.
 * @param i		Returned block will be 2^i big.
 *
 * @return		Block of data represented by link_t.
 */
link_t *buddy_system_alloc(buddy_system_t *b, uint8_t i)
{
	link_t *res, *hlp;

	ASSERT(i <= b->max_order);

	/*
	 * If the list of order i is not empty,
	 * the request can be immediatelly satisfied.
	 */
	res = list_first(&b->order[i]);
	if (res != NULL) {
		list_remove(res);
		b->op->mark_busy(b, res);
		return res;
	}
	/*
	 * If order i is already the maximal order,
	 * the request cannot be satisfied.
	 */
	if (i == b->max_order)
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
	 * Return the other half to buddy system. Mark the first part
	 * full, so that it won't coalesce again.
	 */
	b->op->mark_busy(b, res);
	buddy_system_free(b, hlp);
	
	return res;
}

/** Return block to buddy system.
 *
 * @param b		Buddy system pointer.
 * @param block		Block to return.
 */
void buddy_system_free(buddy_system_t *b, link_t *block)
{
	link_t *buddy, *hlp;
	uint8_t i;

	/*
	 * Determine block's order.
	 */
	i = b->op->get_order(b, block);

	ASSERT(i <= b->max_order);

	if (i != b->max_order) {
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
			 * Recursively add the coalesced block to the list of
			 * order i + 1.
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

/** @}
 */
