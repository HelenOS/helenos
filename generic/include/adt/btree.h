/*
 * Copyright (C) 2006 Jakub Jermar
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

#ifndef __BTREE_H__
#define __BTREE_H__

#include <arch/types.h>
#include <typedefs.h>
#include <adt/list.h>

#define BTREE_M		5
#define BTREE_MAX_KEYS	(BTREE_M - 1)

/** B-tree node structure. */
struct btree_node {
	/** Number of keys. */
	count_t keys;

	/** Keys. We currently support only single keys. Additional room for one extra key is provided. */
	__native key[BTREE_MAX_KEYS + 1];

	/** 
	 * Pointers to values. Sorted according to the key array. Defined only in leaf-level.
	 * There is room for storing value for the extra key.
	 */
	void *value[BTREE_MAX_KEYS + 1];
	
	/**
	 * Pointers to descendants of this node sorted according to the key array.
	 * subtree[0] points to subtree with keys lesser than to key[0].
	 * subtree[1] points to subtree with keys greater than or equal to key[0] and lesser than key[1].
	 * ...
	 * There is room for storing a subtree pointer for the extra key.
	 */
	btree_node_t *subtree[BTREE_M + 1];

	/** Pointer to parent node. Root node has NULL parent. */
	btree_node_t *parent;

	/** Link connecting leaf-level nodes. Defined only when this node is a leaf. */
	link_t leaf_link;

	/** Variables needed by btree_print(). */	
	link_t bfs_link;
	int depth;
};

/** B-tree structure. */
struct btree {
	btree_node_t *root;	/**< B-tree root node pointer. */
	link_t leaf_head;	/**< Leaf-level list head. */
};

extern void btree_create(btree_t *t);
extern void btree_destroy(btree_t *t);

extern void btree_insert(btree_t *t, __native key, void *value, btree_node_t *leaf_node);
extern void btree_remove(btree_t *t, __native key, btree_node_t *leaf_node);
extern void *btree_search(btree_t *t, __native key, btree_node_t **leaf_node);

extern void btree_print(btree_t *t);
#endif
