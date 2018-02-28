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

/** @addtogroup genericadt
 * @{
 */
/** @file
 */

#ifndef KERN_BTREE_H_
#define KERN_BTREE_H_

#include <adt/list.h>
#include <stddef.h>

#define BTREE_M		5
#define BTREE_MAX_KEYS	(BTREE_M - 1)

typedef uint64_t btree_key_t;

/** B-tree node structure. */
typedef struct btree_node {
	/** Number of keys. */
	size_t keys;

	/**
	 * Keys. We currently support only single keys. Additional room for one
	 * extra key is provided.
	 */
	btree_key_t key[BTREE_MAX_KEYS + 1];

	/**
	 * Pointers to values. Sorted according to the key array. Defined only in
	 * leaf-level. There is room for storing value for the extra key.
	 */
	void *value[BTREE_MAX_KEYS + 1];

	/**
	 * Pointers to descendants of this node sorted according to the key
	 * array.
	 *
	 * subtree[0] points to subtree with keys lesser than to key[0].
	 * subtree[1] points to subtree with keys greater than or equal to
	 *	      key[0] and lesser than key[1].
	 * ...
	 * There is room for storing a subtree pointer for the extra key.
	 */
	struct btree_node *subtree[BTREE_M + 1];

	/** Pointer to parent node. Root node has NULL parent. */
	struct btree_node *parent;

	/**
	 * Link connecting leaf-level nodes. Defined only when this node is a
	 * leaf. */
	link_t leaf_link;

	/* Variables needed by btree_print(). */
	link_t bfs_link;
	int depth;
} btree_node_t;

/** B-tree structure. */
typedef struct {
	btree_node_t *root;	/**< B-tree root node pointer. */
	list_t leaf_list;	/**< List of leaves. */
} btree_t;

extern void btree_init(void);

extern void btree_create(btree_t *t);
extern void btree_destroy(btree_t *t);

extern void btree_insert(btree_t *t, btree_key_t key, void *value,
    btree_node_t *leaf_node);
extern void btree_remove(btree_t *t, btree_key_t key, btree_node_t *leaf_node);
extern void *btree_search(btree_t *t, btree_key_t key, btree_node_t **leaf_node);

extern btree_node_t *btree_leaf_node_left_neighbour(btree_t *t,
    btree_node_t *node);
extern btree_node_t *btree_leaf_node_right_neighbour(btree_t *t,
    btree_node_t *node);

extern void btree_print(btree_t *t);

extern unsigned long btree_count(btree_t *t);

#endif

/** @}
 */
