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

/**
 * @file
 * @brief B+tree implementation.
 *
 * This file implements B+tree type and operations.
 *
 * The B+tree has the following properties:
 * @li it is a balanced 3-4-5 tree (i.e. BTREE_M = 5)
 * @li values (i.e. pointers to values) are stored only in leaves
 * @li leaves are linked in a list
 *
 * Be careful when using these trees. They need to allocate
 * and deallocate memory for their index nodes and as such
 * can sleep.
 */

#include <adt/btree.h>
#include <adt/list.h>
#include <assert.h>
#include <mm/slab.h>
#include <panic.h>
#include <print.h>
#include <trace.h>

static slab_cache_t *btree_node_cache;

#define ROOT_NODE(n)   (!(n)->parent)
#define INDEX_NODE(n)  ((n)->subtree[0] != NULL)
#define LEAF_NODE(n)   ((n)->subtree[0] == NULL)

#define FILL_FACTOR  ((BTREE_M - 1) / 2)

#define MEDIAN_LOW_INDEX(n)   (((n)->keys-1) / 2)
#define MEDIAN_HIGH_INDEX(n)  ((n)->keys / 2)
#define MEDIAN_LOW(n)         ((n)->key[MEDIAN_LOW_INDEX((n))]);
#define MEDIAN_HIGH(n)        ((n)->key[MEDIAN_HIGH_INDEX((n))]);

/** Initialize B-trees. */
void btree_init(void)
{
	btree_node_cache = slab_cache_create("btree_node_t",
	    sizeof(btree_node_t), 0, NULL, NULL, SLAB_CACHE_MAGDEFERRED);
}

/** Initialize B-tree node.
 *
 * @param node B-tree node.
 *
 */
NO_TRACE static void node_initialize(btree_node_t *node)
{
	unsigned int i;

	node->keys = 0;

	/* Clean also space for the extra key. */
	for (i = 0; i < BTREE_MAX_KEYS + 1; i++) {
		node->key[i] = 0;
		node->value[i] = NULL;
		node->subtree[i] = NULL;
	}

	node->subtree[i] = NULL;
	node->parent = NULL;

	link_initialize(&node->leaf_link);
	link_initialize(&node->bfs_link);
	node->depth = 0;
}

/** Create empty B-tree.
 *
 * @param t B-tree.
 *
 */
void btree_create(btree_t *t)
{
	list_initialize(&t->leaf_list);
	t->root = (btree_node_t *) slab_alloc(btree_node_cache, 0);
	node_initialize(t->root);
	list_append(&t->root->leaf_link, &t->leaf_list);
}

/** Destroy subtree rooted in a node.
 *
 * @param root Root of the subtree.
 *
 */
NO_TRACE static void btree_destroy_subtree(btree_node_t *root)
{
	size_t i;

	if (root->keys) {
		for (i = 0; i < root->keys + 1; i++) {
			if (root->subtree[i])
				btree_destroy_subtree(root->subtree[i]);
		}
	}

	slab_free(btree_node_cache, root);
}

/** Destroy empty B-tree. */
void btree_destroy(btree_t *t)
{
	btree_destroy_subtree(t->root);
}

/** Insert key-value-rsubtree triplet into B-tree node.
 *
 * It is actually possible to have more keys than BTREE_MAX_KEYS.
 * This feature is used during splitting the node when the
 * number of keys is BTREE_MAX_KEYS + 1. Insert by left rotation
 * also makes use of this feature.
 *
 * @param node     B-tree node into which the new key is to be inserted.
 * @param key      The key to be inserted.
 * @param value    Pointer to value to be inserted.
 * @param rsubtree Pointer to the right subtree.
 *
 */
NO_TRACE static void node_insert_key_and_rsubtree(btree_node_t *node,
    btree_key_t key, void *value, btree_node_t *rsubtree)
{
	size_t i;

	for (i = 0; i < node->keys; i++) {
		if (key < node->key[i]) {
			size_t j;

			for (j = node->keys; j > i; j--) {
				node->key[j] = node->key[j - 1];
				node->value[j] = node->value[j - 1];
				node->subtree[j + 1] = node->subtree[j];
			}

			break;
		}
	}

	node->key[i] = key;
	node->value[i] = value;
	node->subtree[i + 1] = rsubtree;
	node->keys++;
}

/** Find key by its left or right subtree.
 *
 * @param node    B-tree node.
 * @param subtree Left or right subtree of a key found in node.
 * @param right   If true, subtree is a right subtree. If false,
 *                subtree is a left subtree.
 *
 * @return Index of the key associated with the subtree.
 *
 */
NO_TRACE static size_t find_key_by_subtree(btree_node_t *node,
    btree_node_t *subtree, bool right)
{
	size_t i;

	for (i = 0; i < node->keys + 1; i++) {
		if (subtree == node->subtree[i])
			return i - (int) (right != false);
	}

	panic("Node %p does not contain subtree %p.", node, subtree);
}

/** Remove key and its left subtree pointer from B-tree node.
 *
 * Remove the key and eliminate gaps in node->key array.
 * Note that the value pointer and the left subtree pointer
 * is removed from the node as well.
 *
 * @param node B-tree node.
 * @param key  Key to be removed.
 *
 */
NO_TRACE static void node_remove_key_and_lsubtree(btree_node_t *node,
    btree_key_t key)
{
	size_t i;
	size_t j;

	for (i = 0; i < node->keys; i++) {
		if (key == node->key[i]) {
			for (j = i + 1; j < node->keys; j++) {
				node->key[j - 1] = node->key[j];
				node->value[j - 1] = node->value[j];
				node->subtree[j - 1] = node->subtree[j];
			}

			node->subtree[j - 1] = node->subtree[j];
			node->keys--;

			return;
		}
	}

	panic("Node %p does not contain key %" PRIu64 ".", node, key);
}

/** Remove key and its right subtree pointer from B-tree node.
 *
 * Remove the key and eliminate gaps in node->key array.
 * Note that the value pointer and the right subtree pointer
 * is removed from the node as well.
 *
 * @param node B-tree node.
 * @param key  Key to be removed.
 *
 */
NO_TRACE static void node_remove_key_and_rsubtree(btree_node_t *node,
    btree_key_t key)
{
	size_t i, j;

	for (i = 0; i < node->keys; i++) {
		if (key == node->key[i]) {
			for (j = i + 1; j < node->keys; j++) {
				node->key[j - 1] = node->key[j];
				node->value[j - 1] = node->value[j];
				node->subtree[j] = node->subtree[j + 1];
			}

			node->keys--;
			return;
		}
	}

	panic("Node %p does not contain key %" PRIu64 ".", node, key);
}

/** Insert key-value-lsubtree triplet into B-tree node.
 *
 * It is actually possible to have more keys than BTREE_MAX_KEYS.
 * This feature is used during insert by right rotation.
 *
 * @param node     B-tree node into which the new key is to be inserted.
 * @param key      The key to be inserted.
 * @param value    Pointer to value to be inserted.
 * @param lsubtree Pointer to the left subtree.
 *
 */
NO_TRACE static void node_insert_key_and_lsubtree(btree_node_t *node,
    btree_key_t key, void *value, btree_node_t *lsubtree)
{
	size_t i;

	for (i = 0; i < node->keys; i++) {
		if (key < node->key[i]) {
			size_t j;

			for (j = node->keys; j > i; j--) {
				node->key[j] = node->key[j - 1];
				node->value[j] = node->value[j - 1];
				node->subtree[j + 1] = node->subtree[j];
			}

			node->subtree[j + 1] = node->subtree[j];
			break;
		}
	}

	node->key[i] = key;
	node->value[i] = value;
	node->subtree[i] = lsubtree;

	node->keys++;
}

/** Rotate one key-value-rsubtree triplet from the left sibling to the right sibling.
 *
 * The biggest key and its value and right subtree is rotated
 * from the left node to the right. If the node is an index node,
 * than the parent node key belonging to the left node takes part
 * in the rotation.
 *
 * @param lnode Left sibling.
 * @param rnode Right sibling.
 * @param idx   Index of the parent node key that is taking part
 *              in the rotation.
 *
 */
NO_TRACE static void rotate_from_left(btree_node_t *lnode, btree_node_t *rnode,
    size_t idx)
{
	btree_key_t key = lnode->key[lnode->keys - 1];

	if (LEAF_NODE(lnode)) {
		void *value = lnode->value[lnode->keys - 1];

		node_remove_key_and_rsubtree(lnode, key);
		node_insert_key_and_lsubtree(rnode, key, value, NULL);
		lnode->parent->key[idx] = key;
	} else {
		btree_node_t *rsubtree = lnode->subtree[lnode->keys];

		node_remove_key_and_rsubtree(lnode, key);
		node_insert_key_and_lsubtree(rnode, lnode->parent->key[idx], NULL, rsubtree);
		lnode->parent->key[idx] = key;

		/* Fix parent link of the reconnected right subtree. */
		rsubtree->parent = rnode;
	}
}

/** Rotate one key-value-lsubtree triplet from the right sibling to the left sibling.
 *
 * The smallest key and its value and left subtree is rotated
 * from the right node to the left. If the node is an index node,
 * than the parent node key belonging to the right node takes part
 * in the rotation.
 *
 * @param lnode Left sibling.
 * @param rnode Right sibling.
 * @param idx   Index of the parent node key that is taking part
 *              in the rotation.
 *
 */
NO_TRACE static void rotate_from_right(btree_node_t *lnode, btree_node_t *rnode,
    size_t idx)
{
	btree_key_t key = rnode->key[0];

	if (LEAF_NODE(rnode)) {
		void *value = rnode->value[0];

		node_remove_key_and_lsubtree(rnode, key);
		node_insert_key_and_rsubtree(lnode, key, value, NULL);
		rnode->parent->key[idx] = rnode->key[0];
	} else {
		btree_node_t *lsubtree = rnode->subtree[0];

		node_remove_key_and_lsubtree(rnode, key);
		node_insert_key_and_rsubtree(lnode, rnode->parent->key[idx], NULL, lsubtree);
		rnode->parent->key[idx] = key;

		/* Fix parent link of the reconnected left subtree. */
		lsubtree->parent = lnode;
	}
}

/** Insert key-value-rsubtree triplet and rotate the node to the left, if this operation can be done.
 *
 * Left sibling of the node (if it exists) is checked for free space.
 * If there is free space, the key is inserted and the smallest key of
 * the node is moved there. The index node which is the parent of both
 * nodes is fixed.
 *
 * @param node     B-tree node.
 * @param inskey   Key to be inserted.
 * @param insvalue Value to be inserted.
 * @param rsubtree Right subtree of inskey.
 *
 * @return True if the rotation was performed, false otherwise.
 *
 */
NO_TRACE static bool try_insert_by_rotation_to_left(btree_node_t *node,
    btree_key_t inskey, void *insvalue, btree_node_t *rsubtree)
{
	size_t idx;
	btree_node_t *lnode;

	/*
	 * If this is root node, the rotation can not be done.
	 */
	if (ROOT_NODE(node))
		return false;

	idx = find_key_by_subtree(node->parent, node, true);
	if ((int) idx == -1) {
		/*
		 * If this node is the leftmost subtree of its parent,
		 * the rotation can not be done.
		 */
		return false;
	}

	lnode = node->parent->subtree[idx];
	if (lnode->keys < BTREE_MAX_KEYS) {
		/*
		 * The rotaion can be done. The left sibling has free space.
		 */
		node_insert_key_and_rsubtree(node, inskey, insvalue, rsubtree);
		rotate_from_right(lnode, node, idx);
		return true;
	}

	return false;
}

/** Insert key-value-rsubtree triplet and rotate the node to the right, if this operation can be done.
 *
 * Right sibling of the node (if it exists) is checked for free space.
 * If there is free space, the key is inserted and the biggest key of
 * the node is moved there. The index node which is the parent of both
 * nodes is fixed.
 *
 * @param node     B-tree node.
 * @param inskey   Key to be inserted.
 * @param insvalue Value to be inserted.
 * @param rsubtree Right subtree of inskey.
 *
 * @return True if the rotation was performed, false otherwise.
 *
 */
NO_TRACE static bool try_insert_by_rotation_to_right(btree_node_t *node,
    btree_key_t inskey, void *insvalue, btree_node_t *rsubtree)
{
	size_t idx;
	btree_node_t *rnode;

	/*
	 * If this is root node, the rotation can not be done.
	 */
	if (ROOT_NODE(node))
		return false;

	idx = find_key_by_subtree(node->parent, node, false);
	if (idx == node->parent->keys) {
		/*
		 * If this node is the rightmost subtree of its parent,
		 * the rotation can not be done.
		 */
		return false;
	}

	rnode = node->parent->subtree[idx + 1];
	if (rnode->keys < BTREE_MAX_KEYS) {
		/*
		 * The rotation can be done. The right sibling has free space.
		 */
		node_insert_key_and_rsubtree(node, inskey, insvalue, rsubtree);
		rotate_from_left(node, rnode, idx);
		return true;
	}

	return false;
}

/** Split full B-tree node and insert new key-value-right-subtree triplet.
 *
 * This function will split a node and return a pointer to a newly created
 * node containing keys greater than or equal to the greater of medians
 * (or median) of the old keys and the newly added key. It will also write
 * the median key to a memory address supplied by the caller.
 *
 * If the node being split is an index node, the median will not be
 * included in the new node. If the node is a leaf node,
 * the median will be copied there.
 *
 * @param node     B-tree node which is going to be split.
 * @param key      The key to be inserted.
 * @param value    Pointer to the value to be inserted.
 * @param rsubtree Pointer to the right subtree of the key being added.
 * @param median   Address in memory, where the median key will be stored.
 *
 * @return Newly created right sibling of node.
 *
 */
NO_TRACE static btree_node_t *node_split(btree_node_t *node, btree_key_t key,
    void *value, btree_node_t *rsubtree, btree_key_t *median)
{
	btree_node_t *rnode;
	size_t i;
	size_t j;

	assert(median);
	assert(node->keys == BTREE_MAX_KEYS);

	/*
	 * Use the extra space to store the extra node.
	 */
	node_insert_key_and_rsubtree(node, key, value, rsubtree);

	/*
	 * Compute median of keys.
	 */
	*median = MEDIAN_HIGH(node);

	/*
	 * Allocate and initialize new right sibling.
	 */
	rnode = (btree_node_t *) slab_alloc(btree_node_cache, 0);
	node_initialize(rnode);
	rnode->parent = node->parent;
	rnode->depth = node->depth;

	/*
	 * Copy big keys, values and subtree pointers to the new right sibling.
	 * If this is an index node, do not copy the median.
	 */
	i = (size_t) INDEX_NODE(node);
	for (i += MEDIAN_HIGH_INDEX(node), j = 0; i < node->keys; i++, j++) {
		rnode->key[j] = node->key[i];
		rnode->value[j] = node->value[i];
		rnode->subtree[j] = node->subtree[i];

		/*
		 * Fix parent links in subtrees.
		 */
		if (rnode->subtree[j])
			rnode->subtree[j]->parent = rnode;
	}

	rnode->subtree[j] = node->subtree[i];
	if (rnode->subtree[j])
		rnode->subtree[j]->parent = rnode;

	rnode->keys = j;  /* Set number of keys of the new node. */
	node->keys /= 2;  /* Shrink the old node. */

	return rnode;
}

/** Recursively insert into B-tree.
 *
 * @param t        B-tree.
 * @param key      Key to be inserted.
 * @param value    Value to be inserted.
 * @param rsubtree Right subtree of the inserted key.
 * @param node     Start inserting into this node.
 *
 */
NO_TRACE static void _btree_insert(btree_t *t, btree_key_t key, void *value,
    btree_node_t *rsubtree, btree_node_t *node)
{
	if (node->keys < BTREE_MAX_KEYS) {
		/*
		 * Node contains enough space, the key can be stored immediately.
		 */
		node_insert_key_and_rsubtree(node, key, value, rsubtree);
	} else if (try_insert_by_rotation_to_left(node, key, value, rsubtree)) {
		/*
		 * The key-value-rsubtree triplet has been inserted because
		 * some keys could have been moved to the left sibling.
		 */
	} else if (try_insert_by_rotation_to_right(node, key, value, rsubtree)) {
		/*
		 * The key-value-rsubtree triplet has been inserted because
		 * some keys could have been moved to the right sibling.
		 */
	} else {
		btree_node_t *rnode;
		btree_key_t median;

		/*
		 * Node is full and both siblings (if both exist) are full too.
		 * Split the node and insert the smallest key from the node containing
		 * bigger keys (i.e. the new node) into its parent.
		 */

		rnode = node_split(node, key, value, rsubtree, &median);

		if (LEAF_NODE(node)) {
			list_insert_after(&rnode->leaf_link, &node->leaf_link);
		}

		if (ROOT_NODE(node)) {
			/*
			 * We split the root node. Create new root.
			 */
			t->root = (btree_node_t *) slab_alloc(btree_node_cache, 0);
			node->parent = t->root;
			rnode->parent = t->root;
			node_initialize(t->root);

			/*
			 * Left-hand side subtree will be the old root (i.e. node).
			 * Right-hand side subtree will be rnode.
			 */
			t->root->subtree[0] = node;

			t->root->depth = node->depth + 1;
		}
		_btree_insert(t, median, NULL, rnode, node->parent);
	}
}

/** Insert key-value pair into B-tree.
 *
 * @param t         B-tree.
 * @param key       Key to be inserted.
 * @param value     Value to be inserted.
 * @param leaf_node Leaf node where the insertion should begin.
 *
 */
void btree_insert(btree_t *t, btree_key_t key, void *value,
    btree_node_t *leaf_node)
{
	btree_node_t *lnode;

	assert(value);

	lnode = leaf_node;
	if (!lnode) {
		if (btree_search(t, key, &lnode))
			panic("B-tree %p already contains key %" PRIu64 ".", t, key);
	}

	_btree_insert(t, key, value, NULL, lnode);
}

/** Rotate in a key from the left sibling or from the index node, if this operation can be done.
 *
 * @param rnode Node into which to add key from its left sibling
 *              or from the index node.
 *
 * @return True if the rotation was performed, false otherwise.
 *
 */
NO_TRACE static bool try_rotation_from_left(btree_node_t *rnode)
{
	size_t idx;
	btree_node_t *lnode;

	/*
	 * If this is root node, the rotation can not be done.
	 */
	if (ROOT_NODE(rnode))
		return false;

	idx = find_key_by_subtree(rnode->parent, rnode, true);
	if ((int) idx == -1) {
		/*
		 * If this node is the leftmost subtree of its parent,
		 * the rotation can not be done.
		 */
		return false;
	}

	lnode = rnode->parent->subtree[idx];
	if (lnode->keys > FILL_FACTOR) {
		rotate_from_left(lnode, rnode, idx);
		return true;
	}

	return false;
}

/** Rotate in a key from the right sibling or from the index node, if this operation can be done.
 *
 * @param lnode Node into which to add key from its right sibling
 *              or from the index node.
 *
 * @return True if the rotation was performed, false otherwise.
 *
 */
NO_TRACE static bool try_rotation_from_right(btree_node_t *lnode)
{
	size_t idx;
	btree_node_t *rnode;

	/*
	 * If this is root node, the rotation can not be done.
	 */
	if (ROOT_NODE(lnode))
		return false;

	idx = find_key_by_subtree(lnode->parent, lnode, false);
	if (idx == lnode->parent->keys) {
		/*
		 * If this node is the rightmost subtree of its parent,
		 * the rotation can not be done.
		 */
		return false;
	}

	rnode = lnode->parent->subtree[idx + 1];
	if (rnode->keys > FILL_FACTOR) {
		rotate_from_right(lnode, rnode, idx);
		return true;
	}

	return false;
}

/** Combine node with any of its siblings.
 *
 * The siblings are required to be below the fill factor.
 *
 * @param node Node to combine with one of its siblings.
 *
 * @return Pointer to the rightmost of the two nodes.
 *
 */
NO_TRACE static btree_node_t *node_combine(btree_node_t *node)
{
	size_t idx;
	btree_node_t *rnode;
	size_t i;

	assert(!ROOT_NODE(node));

	idx = find_key_by_subtree(node->parent, node, false);
	if (idx == node->parent->keys) {
		/*
		 * Rightmost subtree of its parent, combine with the left sibling.
		 */
		idx--;
		rnode = node;
		node = node->parent->subtree[idx];
	} else
		rnode = node->parent->subtree[idx + 1];

	/* Index nodes need to insert parent node key in between left and right node. */
	if (INDEX_NODE(node))
		node->key[node->keys++] = node->parent->key[idx];

	/* Copy the key-value-subtree triplets from the right node. */
	for (i = 0; i < rnode->keys; i++) {
		node->key[node->keys + i] = rnode->key[i];
		node->value[node->keys + i] = rnode->value[i];

		if (INDEX_NODE(node)) {
			node->subtree[node->keys + i] = rnode->subtree[i];
			rnode->subtree[i]->parent = node;
		}
	}

	if (INDEX_NODE(node)) {
		node->subtree[node->keys + i] = rnode->subtree[i];
		rnode->subtree[i]->parent = node;
	}

	node->keys += rnode->keys;
	return rnode;
}

/** Recursively remove B-tree node.
 *
 * @param t    B-tree.
 * @param key  Key to be removed from the B-tree along with its associated value.
 * @param node Node where the key being removed resides.
 *
 */
NO_TRACE static void _btree_remove(btree_t *t, btree_key_t key,
    btree_node_t *node)
{
	if (ROOT_NODE(node)) {
		if ((node->keys == 1) && (node->subtree[0])) {
			/*
			 * Free the current root and set new root.
			 */
			t->root = node->subtree[0];
			t->root->parent = NULL;
			slab_free(btree_node_cache, node);
		} else {
			/*
			 * Remove the key from the root node.
			 * Note that the right subtree is removed because when
			 * combining two nodes, the left-side sibling is preserved
			 * and the right-side sibling is freed.
			 */
			node_remove_key_and_rsubtree(node, key);
		}

		return;
	}

	if (node->keys <= FILL_FACTOR) {
		/*
		 * If the node is below the fill factor,
		 * try to borrow keys from left or right sibling.
		 */
		if (!try_rotation_from_left(node))
			try_rotation_from_right(node);
	}

	if (node->keys > FILL_FACTOR) {
		size_t i;

		/*
		 * The key can be immediately removed.
		 *
		 * Note that the right subtree is removed because when
		 * combining two nodes, the left-side sibling is preserved
		 * and the right-side sibling is freed.
		 */
		node_remove_key_and_rsubtree(node, key);

		for (i = 0; i < node->parent->keys; i++) {
			if (node->parent->key[i] == key)
				node->parent->key[i] = node->key[0];
		}
	} else {
		size_t idx;
		btree_node_t *rnode;
		btree_node_t *parent;

		/*
		 * The node is below the fill factor as well as its left and right sibling.
		 * Resort to combining the node with one of its siblings.
		 * The node which is on the left is preserved and the node on the right is
		 * freed.
		 */
		parent = node->parent;
		node_remove_key_and_rsubtree(node, key);
		rnode = node_combine(node);

		if (LEAF_NODE(rnode))
			list_remove(&rnode->leaf_link);

		idx = find_key_by_subtree(parent, rnode, true);
		assert((int) idx != -1);
		slab_free(btree_node_cache, rnode);
		_btree_remove(t, parent->key[idx], parent);
	}
}

/** Remove B-tree node.
 *
 * @param t         B-tree.
 * @param key       Key to be removed from the B-tree along
 *                  with its associated value.
 * @param leaf_node If not NULL, pointer to the leaf node where
 *                  the key is found.
 *
 */
void btree_remove(btree_t *t, btree_key_t key, btree_node_t *leaf_node)
{
	btree_node_t *lnode;

	lnode = leaf_node;
	if (!lnode) {
		if (!btree_search(t, key, &lnode))
			panic("B-tree %p does not contain key %" PRIu64 ".", t, key);
	}

	_btree_remove(t, key, lnode);
}

/** Search key in a B-tree.
 *
 * @param t         B-tree.
 * @param key       Key to be searched.
 * @param leaf_node Address where to put pointer to visited leaf node.
 *
 * @return Pointer to value or NULL if there is no such key.
 *
 */
void *btree_search(btree_t *t, btree_key_t key, btree_node_t **leaf_node)
{
	btree_node_t *cur, *next;
	bool descend;

	/*
	 * Iteratively descend to the leaf that can contain the searched key.
	 */
	for (cur = t->root; cur; cur = next) {
		/*
		 * Last iteration will set this with proper
		 * leaf node address.
		 */
		*leaf_node = cur;

		if (cur->keys == 0)
			return NULL;

		/*
		 * The key can be in the leftmost subtree.
		 * Test it separately.
		 */
		if (key < cur->key[0]) {
			next = cur->subtree[0];
			continue;
		} else {
			void *val;
			size_t i;

			/*
			 * Now if the key is smaller than cur->key[i]
			 * it can only mean that the value is in cur->subtree[i]
			 * or it is not in the tree at all.
			 */
			descend = false;
			for (i = 1; i < cur->keys; i++) {
				if (key < cur->key[i]) {
					next = cur->subtree[i];
					val = cur->value[i - 1];

					if (LEAF_NODE(cur))
						return key == cur->key[i - 1] ? val : NULL;

					descend = true;
					break;
				}
			}

			if (descend)
				continue;

			/*
			 * Last possibility is that the key is
			 * in the rightmost subtree.
			 */
			next = cur->subtree[i];
			val = cur->value[i - 1];

			if (LEAF_NODE(cur))
				return key == cur->key[i - 1] ? val : NULL;
		}
	}

	/*
	 * The key was not found in the *leaf_node and
	 * is smaller than any of its keys.
	 */
	return NULL;
}

/** Return pointer to B-tree leaf node's left neighbour.
 *
 * @param t    B-tree.
 * @param node Node whose left neighbour will be returned.
 *
 * @return Left neighbour of the node or NULL if the node
 *         does not have the left neighbour.
 *
 */
btree_node_t *btree_leaf_node_left_neighbour(btree_t *t, btree_node_t *node)
{
	assert(LEAF_NODE(node));

	if (node->leaf_link.prev != &t->leaf_list.head)
		return list_get_instance(node->leaf_link.prev, btree_node_t, leaf_link);
	else
		return NULL;
}

/** Return pointer to B-tree leaf node's right neighbour.
 *
 * @param t    B-tree.
 * @param node Node whose right neighbour will be returned.
 *
 * @return Right neighbour of the node or NULL if the node
 *         does not have the right neighbour.
 *
 */
btree_node_t *btree_leaf_node_right_neighbour(btree_t *t, btree_node_t *node)
{
	assert(LEAF_NODE(node));

	if (node->leaf_link.next != &t->leaf_list.head)
		return list_get_instance(node->leaf_link.next, btree_node_t, leaf_link);
	else
		return NULL;
}

/** Print B-tree.
 *
 * @param t Print out B-tree.
 *
 */
void btree_print(btree_t *t)
{
	size_t i;
	int depth = t->root->depth;
	list_t list;

	printf("Printing B-tree:\n");
	list_initialize(&list);
	list_append(&t->root->bfs_link, &list);

	/*
	 * Use BFS search to print out the tree.
	 * Levels are distinguished from one another by node->depth.
	 */
	while (!list_empty(&list)) {
		link_t *hlp;
		btree_node_t *node;

		hlp = list_first(&list);
		assert(hlp != NULL);
		node = list_get_instance(hlp, btree_node_t, bfs_link);
		list_remove(hlp);

		assert(node);

		if (node->depth != depth) {
			printf("\n");
			depth = node->depth;
		}

		printf("(");

		for (i = 0; i < node->keys; i++) {
			printf("%" PRIu64 "%s", node->key[i], i < node->keys - 1 ? "," : "");
			if (node->depth && node->subtree[i]) {
				list_append(&node->subtree[i]->bfs_link, &list);
			}
		}

		if (node->depth && node->subtree[i])
			list_append(&node->subtree[i]->bfs_link, &list);

		printf(")");
	}

	printf("\n");

	printf("Printing list of leaves:\n");
	list_foreach(t->leaf_list, leaf_link, btree_node_t, node) {
		assert(node);

		printf("(");

		for (i = 0; i < node->keys; i++)
			printf("%" PRIu64 "%s", node->key[i], i < node->keys - 1 ? "," : "");

		printf(")");
	}

	printf("\n");
}

/** Return number of B-tree elements.
 *
 * @param t B-tree to count.
 *
 * @return Return number of B-tree elements.
 *
 */
unsigned long btree_count(btree_t *t)
{
	unsigned long count = 0;

	list_foreach(t->leaf_list, leaf_link, btree_node_t, node) {
		count += node->keys;
	}

	return count;
}

/** @}
 */
