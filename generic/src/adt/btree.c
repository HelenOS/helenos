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

/*
 * This B-tree has the following properties:
 * - it is a ballanced 2-3-4 tree (i.e. BTREE_M = 4)
 * - values (i.e. pointers to values) are stored only in leaves
 * - leaves are linked in a list
 * - technically, it is a B+-tree (because of the previous properties)
 *
 * Some of the functions below take pointer to the right-hand
 * side subtree pointer as parameter. Note that this is sufficient
 * because:
 * 	- New root node is passed the left-hand side subtree pointer
 * 	  directly.
 * 	- node_split() always creates the right sibling and preserves
 * 	  the original node (which becomes the left sibling).
 * 	  There is always pointer to the left-hand side subtree
 * 	  (i.e. left sibling) in the parent node.
 */

#include <adt/btree.h>
#include <adt/list.h>
#include <mm/slab.h>
#include <debug.h>
#include <panic.h>
#include <typedefs.h>
#include <print.h>

static void _btree_insert(btree_t *t, __native key, void *value, btree_node_t *rsubtree, btree_node_t *node);
static void node_initialize(btree_node_t *node);
static void node_insert_key(btree_node_t *node, __native key, void *value, btree_node_t *rsubtree);
static btree_node_t *node_split(btree_node_t *node, __native key, void *value, btree_node_t *rsubtree, __native *median);

#define ROOT_NODE(n)		(!(n)->parent)
#define INDEX_NODE(n)		((n)->subtree[0] != NULL)
#define LEAF_NODE(n)		((n)->subtree[0] == NULL)

#define MEDIAN_LOW_INDEX(n)	(((n)->keys-1)/2)
#define MEDIAN_HIGH_INDEX(n)	((n)->keys/2)
#define MEDIAN_LOW(n)		((n)->key[MEDIAN_LOW_INDEX((n))]);
#define MEDIAN_HIGH(n)		((n)->key[MEDIAN_HIGH_INDEX((n))]);

/** Create empty B-tree.
 *
 * @param t B-tree.
 */
void btree_create(btree_t *t)
{
	list_initialize(&t->leaf_head);
	t->root = (btree_node_t *) malloc(sizeof(btree_node_t), 0);
	node_initialize(t->root);
	list_append(&t->root->leaf_link, &t->leaf_head);
}

/** Destroy empty B-tree. */
void btree_destroy(btree_t *t)
{
	ASSERT(!t->root->keys);
	free(t->root);
}

/** Insert key-value pair into B-tree.
 *
 * @param t B-tree.
 * @param key Key to be inserted.
 * @param value Value to be inserted.
 * @param leaf_node Leaf node where the insertion should begin.
 */ 
void btree_insert(btree_t *t, __native key, void *value, btree_node_t *leaf_node)
{
	btree_node_t *lnode;
	
	ASSERT(value);
	
	lnode = leaf_node;
	if (!lnode) {
		if (btree_search(t, key, &lnode)) {
			panic("B-tree %P already contains key %d\n", t, key);
		}
	}
	
	_btree_insert(t, key, value, NULL, lnode);
}

/** Recursively insert into B-tree.
 *
 * @param t B-tree.
 * @param key Key to be inserted.
 * @param value Value to be inserted.
 * @param rsubtree Right subtree of the inserted key.
 * @param node Start inserting into this node.
 */
void _btree_insert(btree_t *t, __native key, void *value, btree_node_t *rsubtree, btree_node_t *node)
{
	if (node->keys < BTREE_MAX_KEYS) {
		/*
		 * Node conatins enough space, the key can be stored immediately.
		 */
		node_insert_key(node, key, value, rsubtree);
	} else {
		btree_node_t *rnode;
		__native median;
		
		/*
		 * Node is full.
		 * Split it and insert the smallest key from the node containing
		 * bigger keys (i.e. the original node) into its parent.
		 */

		rnode = node_split(node, key, value, rsubtree, &median);

		if (LEAF_NODE(node)) {
			list_append(&rnode->leaf_link, &node->leaf_link);
		}
		
		if (ROOT_NODE(node)) {
			/*
			 * We split the root node. Create new root.
			 */
		
			t->root = (btree_node_t *) malloc(sizeof(btree_node_t), 0);
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

/* TODO */
void btree_remove(btree_t *t, __native key)
{
}

/** Search key in a B-tree.
 *
 * @param t B-tree.
 * @param key Key to be searched.
 * @param leaf_node Address where to put pointer to visited leaf node.
 *
 * @return Pointer to value or NULL if there is no such key.
 */
void *btree_search(btree_t *t, __native key, btree_node_t **leaf_node)
{
	btree_node_t *cur, *next;
	void *val = NULL;
	
	/*
	 * Iteratively descend to the leaf that can contain searched key.
	 */
	for (cur = t->root; cur; cur = next) {
		int i;
	
		/* Last iteration will set this with proper leaf node address. */
		*leaf_node = cur;
		for (i = 0; i < cur->keys; i++) {
			if (key <= cur->key[i]) {
				val = cur->value[i];
				next = cur->subtree[i];
				
				/*
				 * Check if there is anywhere to descend.
				 */
				if (!next) {
					/*
					 * Leaf-level.
					 */
					return (key == cur->key[i]) ? val : NULL;
				}
				goto descend;
			}
		}
		next = cur->subtree[i];
	descend:
		;
	}

	/*
	 * The key was not found in the *leaf_node and is greater than any of its keys.
	 */
	return NULL;
}

/** Get pointer to value with the smallest key within the node.
 *
 * Can be only used on leaf-level nodes.
 *
 * @param node B-tree node.
 *
 * @return Pointer to value assiciated with the smallest key.
 */
void *btree_node_min(btree_node_t *node)
{
	ASSERT(LEAF_NODE(node));
	ASSERT(node->keys);
	return node->value[0];
}

/** Get pointer to value with the biggest key within the node.
 *
 * Can be only used on leaf-level nodes.
 *
 * @param node B-tree node.
 *
 * @return Pointer to value assiciated with the biggest key.
 */
void *btree_node_max(btree_node_t *node)
{
	ASSERT(LEAF_NODE(node));
	ASSERT(node->keys);
	return node->value[node->keys - 1];
}

/** Initialize B-tree node.
 *
 * @param node B-tree node.
 */
void node_initialize(btree_node_t *node)
{
	int i;

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

/** Insert key-value-left-subtree triplet into B-tree non-full node.
 *
 * It is actually possible to have more keys than BTREE_MAX_KEYS.
 * This feature is used during splitting the node when the
 * number of keys is BTREE_MAX_KEYS + 1.
 *
 * @param node B-tree node into wich the new key is to be inserted.
 * @param key The key to be inserted.
 * @param value Pointer to value to be inserted.
 * @param rsubtree Pointer to the right subtree.
 */ 
void node_insert_key(btree_node_t *node, __native key, void *value, btree_node_t *rsubtree)
{
	int i;

	for (i = 0; i < node->keys; i++) {
		if (key < node->key[i]) {
			int j;
		
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

/** Split full B-tree node and insert new key-value-left-subtree triplet.
 *
 * This function will split a node and return pointer to a newly created
 * node containing keys greater than the lesser of medians (or median)
 * of the old keys and the newly added key. It will also write the
 * median key to a memory address supplied by the caller.
 *
 * If the node being split is an index node, the median will be
 * removed from the original node. If the node is a leaf node,
 * the median will be preserved.
 *
 * @param node B-tree node wich is going to be split.
 * @param key The key to be inserted.
 * @param value Pointer to the value to be inserted.
 * @param rsubtree Pointer to the right subtree of the key being added.
 * @param median Address in memory, where the median key will be stored.
 *
 * @return Newly created right sibling of node.
 */ 
btree_node_t *node_split(btree_node_t *node, __native key, void *value, btree_node_t *rsubtree, __native *median)
{
	btree_node_t *rnode;
	int i, j;

	ASSERT(median);
	ASSERT(node->keys == BTREE_MAX_KEYS);
	
	/*
	 * Use the extra space to store the extra node.
	 */
	node_insert_key(node, key, value, rsubtree);

	/*
	 * Compute median of keys.
	 */
	*median = MEDIAN_LOW(node);
		
	rnode = (btree_node_t *) malloc(sizeof(btree_node_t), 0);
	node_initialize(rnode);
	rnode->parent = node->parent;
	rnode->depth = node->depth;
	
	/*
	 * Copy big keys, values and subtree pointers to the new right sibling.
	 */
	for (i = MEDIAN_LOW_INDEX(node) + 1, j = 0; i < node->keys; i++, j++) {
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
	rnode->keys = j;
	
	/*
	 * Shrink the old node.
	 * If this is an index node, remove the median.
	 */
	node->keys = MEDIAN_LOW_INDEX(node) + 1;
	if (INDEX_NODE(node))
		node->keys--;
		
	return rnode;
}

/** Print B-tree.
 *
 * @param t Print out B-tree.
 */
void btree_print(btree_t *t)
{
	int i, depth = t->root->depth;
	link_t head;
	
	list_initialize(&head);
	list_append(&t->root->bfs_link, &head);

	/*
	 * Use BFS search to print out the tree.
	 * Levels are distinguished from one another by node->depth.
	 */	
	while (!list_empty(&head)) {
		link_t *hlp;
		btree_node_t *node;
		
		hlp = head.next;
		ASSERT(hlp != &head);
		node = list_get_instance(hlp, btree_node_t, bfs_link);
		list_remove(hlp);
		
		ASSERT(node);
		
		if (node->depth != depth) {
			printf("\n");
			depth = node->depth;
		}

		printf("(");
		for (i = 0; i < node->keys; i++) {
			printf("%d,", node->key[i]);
			if (node->depth && node->subtree[i]) {
				list_append(&node->subtree[i]->bfs_link, &head);
			}
		}
		if (node->depth && node->subtree[i]) {
			list_append(&node->subtree[i]->bfs_link, &head);
		}
		printf(")");
	}
	printf("\n");
}
