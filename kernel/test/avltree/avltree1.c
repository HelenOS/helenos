/*
 * Copyright (c) 2007 Vojtech Mencl
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

#include <test.h>
#include <print.h>
#include <adt/avl.h>
#include <debug.h>
#include <typedefs.h>

#define NODE_COUNT 100

static avltree_t avltree;

/*
 * avl tree nodes in array for faster allocation
 */
static avltree_node_t avltree_nodes[NODE_COUNT];

/*
 * head of free nodes' list:
 */
static avltree_node_t *first_free_node = NULL;

static int test_tree_balance(avltree_node_t *node);
static avltree_node_t *test_tree_parents(avltree_node_t *node);
static void print_tree_structure_flat (avltree_node_t *node, int level)
    __attribute__ ((used));
static avltree_node_t *alloc_avltree_node(void);

static avltree_node_t *test_tree_parents(avltree_node_t *node)
{
	avltree_node_t *tmp;

	if (!node)
		return NULL;

	if (node->lft) {
		tmp = test_tree_parents(node->lft);
		if (tmp != node) {
			TPRINTF("Bad parent pointer key: %" PRIu64
			    ", address: %p\n", tmp->key, node->lft);
		}
	}
	if (node->rgt) {
		tmp = test_tree_parents(node->rgt);
		if (tmp != node) {
			TPRINTF("Bad parent pointer key: %" PRIu64
			    ", address: %p\n",
			    tmp->key,node->rgt);
		}
	}
	return node->par;
}

int test_tree_balance(avltree_node_t *node)
{
	int h1, h2, diff;

	if (!node)
		return 0;

	h1 = test_tree_balance(node->lft);
	h2 = test_tree_balance(node->rgt);
	diff = h2 - h1;

	if ((diff != node->balance) || ((diff != -1) && (diff != 0) && (diff != 1)))
		TPRINTF("Bad balance\n");

	return ((h1 > h2) ? (h1 + 1) : (h2 + 1));
}

/**
 * Prints the structure of the node, which is level levels from the top of the
 * tree.
 */
static void print_tree_structure_flat(avltree_node_t *node, int level)
{
	/*
	 * You can set the maximum level as high as you like.
	 * Most of the time, you'll want to debug code using small trees,
	 * so that a large level indicates a loop, which is a bug.
	 */
	if (level > 16) {
		TPRINTF("[...]");
		return;
	}

	if (node == NULL)
		return;

	TPRINTF("%" PRIu64 "[%" PRIu8 "]", node->key, node->balance);
	if (node->lft != NULL || node->rgt != NULL) {
		TPRINTF("(");

		print_tree_structure_flat(node->lft, level + 1);
		if (node->rgt != NULL) {
			TPRINTF(",");
			print_tree_structure_flat(node->rgt, level + 1);
		}

		TPRINTF(")");
	}
}

static void alloc_avltree_node_prepare(void)
{
	int i;

	for (i = 0; i < NODE_COUNT - 1; i++)
		avltree_nodes[i].par = &avltree_nodes[i + 1];

	avltree_nodes[i].par = NULL;

	/*
	 * Node keys which will be used for insertion. Up to NODE_COUNT size of
	 * array.
	 */

	/* First tree node and same key */
	avltree_nodes[0].key = 60;
	avltree_nodes[1].key = 60;
	avltree_nodes[2].key = 60;

	/* LL rotation */
	avltree_nodes[3].key = 50;
	avltree_nodes[4].key = 40;
	avltree_nodes[5].key = 30;

	/* LR rotation */
	avltree_nodes[6].key = 20;
	avltree_nodes[7].key = 20;
	avltree_nodes[8].key = 25;
	avltree_nodes[9].key = 25;

	/* LL rotation in lower floor */
	avltree_nodes[10].key = 35;

	/* RR rotation */
	avltree_nodes[11].key = 70;
	avltree_nodes[12].key = 80;

	/* RL rotation */
	avltree_nodes[13].key = 90;
	avltree_nodes[14].key = 85;

	/* Insert 0 key */
	avltree_nodes[15].key = 0;
	avltree_nodes[16].key = 0;

	/* Insert reverse */
	avltree_nodes[17].key = 600;
	avltree_nodes[18].key = 500;
	avltree_nodes[19].key = 400;
	avltree_nodes[20].key = 300;

	for (i = 21; i < NODE_COUNT; i++)
		avltree_nodes[i].key = i * 3;

	first_free_node = &avltree_nodes[0];
}

static avltree_node_t *alloc_avltree_node(void)
{
	avltree_node_t *node;

	node = first_free_node;
	first_free_node = first_free_node->par;

	return node;
}

static void test_tree_insert(avltree_t *tree, size_t node_count)
{
	unsigned int i;
	avltree_node_t *newnode;

	avltree_create(tree);

	TPRINTF("Inserting %zu nodes...", node_count);

	for (i = 0; i < node_count; i++) {
		newnode = alloc_avltree_node();

		avltree_insert(tree, newnode);
		test_tree_parents(tree->root);
		test_tree_balance(tree->root);
	}

	TPRINTF("done.\n");
}

static void test_tree_delete(avltree_t *tree, size_t node_count,
    int node_position)
{
	avltree_node_t *delnode;
	unsigned int i;

	switch (node_position) {
	case 0:
		TPRINTF("Deleting root nodes...");

		while (tree->root != NULL) {
			delnode = tree->root;
			avltree_delete(tree, delnode);
			test_tree_parents(tree->root);
			test_tree_balance(tree->root);
		}
		break;
	case 1:
		TPRINTF("Deleting nodes according to creation time...");

		for (i = 0; i < node_count; i++) {
			avltree_delete(tree, &avltree_nodes[i]);
			test_tree_parents(tree->root);
			test_tree_balance(tree->root);
		}
		break;
	}

	TPRINTF("done.\n");
}

static void test_tree_delmin(avltree_t *tree, size_t node_count)
{
	unsigned int i = 0;

	TPRINTF("Deleting minimum nodes...");

	while (tree->root != NULL) {
		i++;
		avltree_delete_min(tree);
		test_tree_parents(tree->root);
		test_tree_balance(tree->root);
	}

	if (i != node_count)
		TPRINTF("Bad node count. Some nodes have been lost!\n");

	TPRINTF("done.\n");
}

const char *test_avltree1(void)
{
	alloc_avltree_node_prepare();
	test_tree_insert(&avltree, NODE_COUNT);
	test_tree_delete(&avltree, NODE_COUNT, 0);

	alloc_avltree_node_prepare();
	test_tree_insert(&avltree, NODE_COUNT);
	test_tree_delete(&avltree, NODE_COUNT, 1);

	alloc_avltree_node_prepare();
	test_tree_insert(&avltree, NODE_COUNT);
	test_tree_delmin(&avltree, NODE_COUNT);

	return NULL;
}
