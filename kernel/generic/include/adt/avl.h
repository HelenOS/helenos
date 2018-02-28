/*
 * Copyright (C) 2007 Vojtech Mencl
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

#ifndef KERN_AVLTREE_H_
#define KERN_AVLTREE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <trace.h>

/**
 * Macro for getting a pointer to the structure which contains the avltree
 * structure.
 *
 * @param link Pointer to the avltree structure.
 * @param type Name of the outer structure.
 * @param member Name of avltree attribute in the outer structure.
 */
#define avltree_get_instance(node, type, member) \
    ((type *)(((uint8_t *)(node)) - ((uint8_t *) &(((type *) NULL)->member))))

typedef struct avltree_node avltree_node_t;
typedef struct avltree avltree_t;

typedef uint64_t avltree_key_t;

typedef bool (* avltree_walker_t)(avltree_node_t *, void *);

/** AVL tree node structure. */
struct avltree_node
{
	/**
	 * Pointer to the left descendant of this node.
	 *
	 * All keys of nodes in the left subtree are less than the key of this
	 * node.
	 */
	struct avltree_node *lft;

	/**
	 * Pointer to the right descendant of this node.
	 *
	 * All keys of nodes in the right subtree are greater than the key of
	 * this node.
	 */
	struct avltree_node *rgt;

	/** Pointer to the parent node. Root node has NULL parent. */
	struct avltree_node *par;

	/** Node's key. */
	avltree_key_t key;

	/**
	 * Difference between the heights of the left and the right subtree of
	 * this node.
	 */
	int8_t balance;
};

/** AVL tree structure. */
struct avltree
{
	/** AVL root node pointer */
	struct avltree_node *root;

	/**
	 * Base of the tree is a value that is smaller or equal than every value
	 * in the tree (valid for positive keys otherwise ignore this atribute).
	 *
	 * The base is added to the current key when a new node is inserted into
	 * the tree. The base is changed to the key of the node which is deleted
	 * with avltree_delete_min().
	 */
	avltree_key_t base;
};


/** Create empty AVL tree.
 *
 * @param t AVL tree.
 */
NO_TRACE static inline void avltree_create(avltree_t *t)
{
	t->root = NULL;
	t->base = 0;
}

/** Initialize node.
 *
 * @param node Node which is initialized.
 */
NO_TRACE static inline void avltree_node_initialize(avltree_node_t *node)
{
	node->key = 0;
	node->lft = NULL;
	node->rgt = NULL;
	node->par = NULL;
	node->balance = 0;
}

extern avltree_node_t *avltree_find_min(avltree_t *t);
extern avltree_node_t *avltree_search(avltree_t *t, avltree_key_t key);
extern void avltree_insert(avltree_t *t, avltree_node_t *newnode);
extern void avltree_delete(avltree_t *t, avltree_node_t *node);
extern bool avltree_delete_min(avltree_t *t);
extern void avltree_walk(avltree_t *t, avltree_walker_t walker, void *arg);

#endif

/** @}
 */
