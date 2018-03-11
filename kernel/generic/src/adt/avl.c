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

/** @addtogroup genericadt
 * @{
 */

/**
 * @file
 * @brief	AVL tree implementation.
 *
 * This file implements AVL tree type and operations.
 *
 * Implemented AVL tree has the following properties:
 * @li It is a binary search tree with non-unique keys.
 * @li Difference of heights of the left and the right subtree of every node is
 *     one at maximum.
 *
 * Every node has a pointer to its parent which allows insertion of multiple
 * identical keys into the tree.
 *
 * Be careful when using this tree because of the base atribute which is added
 * to every inserted node key. There is no rule in which order nodes with the
 * same key are visited.
 */

#include <adt/avl.h>
#include <assert.h>

#define LEFT 	0
#define RIGHT	1

/** Search for the first occurence of the given key in an AVL tree.
 *
 * @param t AVL tree.
 * @param key Key to be searched.
 *
 * @return Pointer to a node or NULL if there is no such key.
 */
avltree_node_t *avltree_search(avltree_t *t, avltree_key_t key)
{
	avltree_node_t *p;

	/*
	 * Iteratively descend to the leaf that can contain the searched key.
	 */
	p = t->root;
	while (p != NULL) {
		if (p->key > key)
			p = p->lft;
		else if (p->key < key)
			p = p->rgt;
		else
			return p;
	}
	return NULL;
}


/** Find the node with the smallest key in an AVL tree.
 *
 * @param t AVL tree.
 *
 * @return Pointer to a node or NULL if there is no node in the tree.
 */
avltree_node_t *avltree_find_min(avltree_t *t)
{
	avltree_node_t *p = t->root;

	/*
	 * Check whether the tree is empty.
	 */
	if (!p)
		return NULL;

	/*
	 * Iteratively descend to the leftmost leaf in the tree.
	 */
	while (p->lft != NULL)
		p = p->lft;

	return p;
}

#define REBALANCE_INSERT_XX(DIR1, DIR2)		\
	top->DIR1 = par->DIR2;			\
	if (top->DIR1 != NULL)			\
		top->DIR1->par = top;		\
	par->par = top->par;			\
	top->par = par;				\
	par->DIR2 = top;			\
	par->balance = 0;			\
	top->balance = 0;			\
	*dpc = par;

#define REBALANCE_INSERT_LL()		REBALANCE_INSERT_XX(lft, rgt)
#define REBALANCE_INSERT_RR()		REBALANCE_INSERT_XX(rgt, lft)

#define REBALANCE_INSERT_XY(DIR1, DIR2, SGN)	\
	gpa = par->DIR2;			\
	par->DIR2 = gpa->DIR1;			\
	if (gpa->DIR1 != NULL)			\
		gpa->DIR1->par = par;		\
	gpa->DIR1 = par;			\
	par->par = gpa;				\
	top->DIR1 = gpa->DIR2;			\
	if (gpa->DIR2 != NULL)			\
		gpa->DIR2->par = top;		\
	gpa->DIR2 = top;			\
	gpa->par = top->par;			\
	top->par = gpa;				\
						\
	if (gpa->balance == -1 * SGN) {		\
		par->balance = 0;		\
		top->balance = 1 * SGN;		\
	} else if (gpa->balance == 0) {		\
		par->balance = 0;		\
		top->balance = 0;		\
	} else {				\
		par->balance = -1 * SGN;	\
		top->balance = 0;		\
	}					\
	gpa->balance = 0;			\
	*dpc = gpa;

#define REBALANCE_INSERT_LR()		REBALANCE_INSERT_XY(lft, rgt, 1)
#define REBALANCE_INSERT_RL()		REBALANCE_INSERT_XY(rgt, lft, -1)

/** Insert new node into AVL tree.
 *
 * @param t AVL tree.
 * @param newnode New node to be inserted.
 */
void avltree_insert(avltree_t *t, avltree_node_t *newnode)
{
	avltree_node_t *par;
	avltree_node_t *gpa;
	avltree_node_t *top;
	avltree_node_t **dpc;
	avltree_key_t key;

	assert(t);
	assert(newnode);

	/*
	 * Creating absolute key.
	 */
	key = newnode->key + t->base;

	/*
	 * Iteratively descend to the leaf that can contain the new node.
	 * Last node with non-zero balance in the way to leaf is stored as top -
	 * it is a place of possible inbalance.
	 */
	dpc = &t->root;
	gpa = NULL;
	top = t->root;
	while ((par = (*dpc)) != NULL) {
		if (par->balance != 0) {
			top = par;
		}
		gpa = par;
		dpc = par->key > key ? &par->lft: &par->rgt;
	}

	/*
	 * Initialize the new node.
	 */
	newnode->key = key;
	newnode->lft = NULL;
	newnode->rgt = NULL;
	newnode->par = gpa;
	newnode->balance = 0;

	/*
	 * Insert first node into the empty tree.
	 */
	if (t->root == NULL) {
		*dpc = newnode;
		return;
	}

	/*
	 * Insert the new node into the previously found leaf position.
	 */
	*dpc = newnode;

	/*
	 * If the tree contains one node - end.
	 */
	if (top == NULL)
		return;

	/*
	 * Store pointer of top's father which points to the node with
	 * potentially broken balance (top).
	 */
	if (top->par == NULL) {
		dpc = &t->root;
	} else {
		if (top->par->lft == top)
			dpc = &top->par->lft;
		else
			dpc = &top->par->rgt;
	}

	/*
	 * Repair all balances on the way from top node to the newly inserted
	 * node.
	 */
	par = top;
	while (par != newnode) {
		if (par->key > key) {
			par->balance--;
			par = par->lft;
		} else {
			par->balance++;
			par = par->rgt;
		}
	}

	/*
	 * To balance the tree, we must check and balance top node.
	 */
	if (top->balance == -2) {
		par = top->lft;
		if (par->balance == -1) {
			/*
			 * LL rotation.
			 */
			REBALANCE_INSERT_LL();
		} else {
			/*
			 * LR rotation.
			 */
			assert(par->balance == 1);

			REBALANCE_INSERT_LR();
		}
	} else if (top->balance == 2) {
		par = top->rgt;
		if (par->balance == 1) {
			/*
			 * RR rotation.
			 */
			REBALANCE_INSERT_RR();
		} else {
			/*
			 * RL rotation.
			 */
			assert(par->balance == -1);

			REBALANCE_INSERT_RL();
		}
	} else {
		/*
		 * Balance is not broken, insertion is finised.
		 */
		return;
	}

}

/** Repair the tree after reparenting node u.
 *
 * If node u has no parent, mark it as the root of the whole tree. Otherwise
 * node v represents stale address of one of the children of node u's parent.
 * Replace v with w as node u parent's child (for most uses, u and w will be the
 * same).
 *
 * @param t	AVL tree.
 * @param u	Node whose new parent has a stale child pointer.
 * @param v	Stale child of node u's new parent.
 * @param w	New child of node u's new parent.
 * @param dir	If not NULL, address of the variable where to store information
 * 		about whether w replaced v in the left or the right subtree of
 * 		u's new parent.
 * @param ro	Read only operation; do not modify any tree pointers. This is
 *		useful for tracking direction via the dir pointer.
 *
 * @return	Zero if w became the new root of the tree, otherwise return
 * 		non-zero.
 */
static int
repair(avltree_t *t, avltree_node_t *u, avltree_node_t *v, avltree_node_t *w,
    int *dir, int ro)
{
	if (u->par == NULL) {
		if (!ro)
			t->root = w;
		return 0;
	} else {
		if (u->par->lft == v) {
			if (!ro)
				u->par->lft = w;
			if (dir)
				*dir = LEFT;
		} else {
			assert(u->par->rgt == v);
			if (!ro)
				u->par->rgt = w;
			if (dir)
				*dir = RIGHT;
		}
	}
	return 1;
}

#define REBALANCE_DELETE(DIR1, DIR2, SIGN)	\
	if (cur->balance == -1 * SIGN) {	\
		par->balance = 0;		\
		gpa->balance = 1 * SIGN;	\
		if (gpa->DIR1)			\
			gpa->DIR1->par = gpa;	\
		par->DIR2->par = par;		\
	} else if (cur->balance == 0) {		\
		par->balance = 0;		\
		gpa->balance = 0;		\
		if (gpa->DIR1)			\
			gpa->DIR1->par = gpa;	\
		if (par->DIR2)			\
			par->DIR2->par = par; 	\
	} else {				\
		par->balance = -1 * SIGN;	\
		gpa->balance = 0;		\
		if (par->DIR2)			\
			par->DIR2->par = par;	\
		gpa->DIR1->par = gpa;		\
	}					\
	cur->balance = 0;

#define REBALANCE_DELETE_LR()		REBALANCE_DELETE(lft, rgt, 1)
#define REBALANCE_DELETE_RL()		REBALANCE_DELETE(rgt, lft, -1)

/** Delete a node from the AVL tree.
 *
 * Because multiple identical keys are allowed, the parent pointers are
 * essential during deletion.
 *
 * @param t AVL tree structure.
 * @param node Address of the node which will be deleted.
 */
void avltree_delete(avltree_t *t, avltree_node_t *node)
{
	avltree_node_t *cur;
	avltree_node_t *par;
	avltree_node_t *gpa;
	int dir;

	assert(t);
	assert(node);

	if (node->lft == NULL) {
		if (node->rgt) {
			/*
			 * Replace the node with its only right son.
			 *
			 * Balance of the right son will be repaired in the
			 * balancing cycle.
			 */
			cur = node->rgt;
			cur->par = node->par;
			gpa = cur;
			dir = RIGHT;
			cur->balance = node->balance;
		} else {
			if (node->par == NULL) {
				/*
				 * The tree has only one node - it will become
				 * an empty tree and the balancing can end.
				 */
				t->root = NULL;
				return;
			}
			/*
			 * The node has no child, it will be deleted with no
			 * substitution.
			 */
			gpa = node->par;
			cur = NULL;
			dir = (gpa->lft == node) ? LEFT: RIGHT;
		}
	} else {
		/*
		 * The node has the left son. Find a node with the smallest key
		 * in the left subtree and replace the deleted node with that
		 * node.
		 */
		cur = node->lft;
		while (cur->rgt != NULL)
			cur = cur->rgt;

		if (cur != node->lft) {
			/*
			 * The rightmost node of the deleted node's left subtree
			 * was found. Replace the deleted node with this node.
			 * Cutting off of the found node has two cases that
			 * depend on its left son.
			 */
			if (cur->lft) {
				/*
				 * The found node has a left son.
				 */
				gpa = cur->lft;
				gpa->par = cur->par;
				dir = LEFT;
				gpa->balance = cur->balance;
			} else {
				dir = RIGHT;
				gpa = cur->par;
			}
			cur->par->rgt = cur->lft;
			cur->lft = node->lft;
			cur->lft->par = cur;
		} else {
			/*
			 * The left son of the node hasn't got a right son. The
			 * left son will take the deleted node's place.
			 */
			dir = LEFT;
			gpa = cur;
		}
		if (node->rgt)
			node->rgt->par = cur;
		cur->rgt = node->rgt;
		cur->balance = node->balance;
		cur->par = node->par;
	}

	/*
	 * Repair the parent node's pointer which pointed previously to the
	 * deleted node.
	 */
	(void) repair(t, node, node, cur, NULL, false);

	/*
	 * Repair cycle which repairs balances of nodes on the way from from the
	 * cut-off node up to the root.
	 */
	while (true) {
		if (dir == LEFT) {
			/*
			 * Deletion was made in the left subtree.
			 */
			gpa->balance++;
			if (gpa->balance == 1) {
				/*
				 * Stop balancing, the tree is balanced.
				 */
				break;
			} else if (gpa->balance == 2) {
				/*
				 * Bad balance, heights of left and right
				 * subtrees differ more than by one.
				 */
				par = gpa->rgt;

				if (par->balance == -1)	{
					/*
					 * RL rotation.
					 */

					cur = par->lft;
					par->lft = cur->rgt;
					cur->rgt = par;
					gpa->rgt = cur->lft;
					cur->lft = gpa;

					/*
					 * Repair balances and paternity of
					 * children, depending on the balance
					 * factor of the grand child (cur).
					 */
					REBALANCE_DELETE_RL();

					/*
					 * Repair paternity.
					 */
					cur->par = gpa->par;
					gpa->par = cur;
					par->par = cur;

					if (!repair(t, cur, gpa, cur, &dir,
					    false))
						break;
					gpa = cur->par;
				} else {
					/*
					 * RR rotation.
					 */

					gpa->rgt = par->lft;
					if (par->lft)
						par->lft->par = gpa;
					par->lft = gpa;

					/*
					 * Repair paternity.
					 */
					par->par = gpa->par;
					gpa->par = par;

					if (par->balance == 0) {
						/*
						 * The right child of the
						 * balanced node is balanced,
						 * after RR rotation is done,
						 * the whole tree will be
						 * balanced.
						 */
						par->balance = -1;
						gpa->balance = 1;

						(void) repair(t, par, gpa, par,
						    NULL, false);
						break;
					} else {
						par->balance = 0;
						gpa->balance = 0;
						if (!repair(t, par, gpa, par,
						    &dir, false))
							break;
					}
					gpa = par->par;
				}
			} else {
				/*
				 * Repair the pointer which pointed to the
				 * balanced node. If it was root then balancing
				 * is finished else continue with the next
				 * iteration (parent node).
				 */
				if (!repair(t, gpa, gpa, NULL, &dir, true))
					break;
				gpa = gpa->par;
			}
		} else {
			/*
			 * Deletion was made in the right subtree.
			 */
			gpa->balance--;
			if (gpa->balance == -1) {
				/*
				 * Stop balancing, the tree is balanced.
				 */
				break;
			} else if (gpa->balance == -2) {
				/*
				 * Bad balance, heights of left and right
				 * subtrees differ more than by one.
				 */
				par = gpa->lft;

				if (par->balance == 1) {
					/*
					 * LR rotation.
					 */

					cur = par->rgt;
					par->rgt = cur->lft;
					cur->lft = par;
					gpa->lft = cur->rgt;
					cur->rgt = gpa;

					/*
					 * Repair balances and paternity of
					 * children, depending on the balance
					 * factor of the grand child (cur).
					 */
					REBALANCE_DELETE_LR();

					/*
					 * Repair paternity.
					 */
					cur->par = gpa->par;
					gpa->par = cur;
					par->par = cur;

					if (!repair(t, cur, gpa, cur, &dir,
					    false))
						break;
					gpa = cur->par;
				} else {
					/*
					 * LL rotation.
					 */

					gpa->lft = par->rgt;
					if (par->rgt)
						par->rgt->par = gpa;
					par->rgt = gpa;
					/*
					 * Repair paternity.
					 */
					par->par = gpa->par;
					gpa->par = par;

					if (par->balance == 0) {
						/*
						 * The left child of the
						 * balanced node is balanced,
						 * after LL rotation is done,
						 * the whole tree will be
						 * balanced.
						 */
						par->balance = 1;
						gpa->balance = -1;

						(void) repair(t, par, gpa, par,
						    NULL, false);
						break;
					} else {
						par->balance = 0;
						gpa->balance = 0;

						if (!repair(t, par, gpa, par,
						    &dir, false))
							break;
					}
					gpa = par->par;
				}
			} else {
				/*
				 * Repair the pointer which pointed to the
				 * balanced node. If it was root then balancing
				 * is finished. Otherwise continue with the next
				 * iteration (parent node).
				 */
				if (!repair(t, gpa, gpa, NULL, &dir, true))
					break;
				gpa = gpa->par;
			}
		}
	}
}


/** Delete a node with the smallest key from the AVL tree.
 *
 * @param t AVL tree structure.
 */
bool avltree_delete_min(avltree_t *t)
{
	avltree_node_t *node;

	/*
	 * Start searching for the smallest key in the tree starting in the root
	 * node and continue in cycle to the leftmost node in the tree (which
	 * must have the smallest key).
	 */

	node = t->root;
	if (!node)
		return false;

	while (node->lft != NULL)
		node = node->lft;

	avltree_delete(t, node);

	return true;
}

/** Walk a subtree of an AVL tree in-order and apply a supplied walker on each
 * visited node.
 *
 * @param node		Node representing the root of an AVL subtree to be
 * 			walked.
 * @param walker	Walker function that will be appliad on each visited
 * 			node.
 * @param arg		Argument for the walker.
 *
 * @return		Zero if the walk should stop or non-zero otherwise.
 */
static bool _avltree_walk(avltree_node_t *node, avltree_walker_t walker,
    void *arg)
{
	if (node->lft) {
		if (!_avltree_walk(node->lft, walker, arg))
			return false;
	}
	if (!walker(node, arg))
		return false;
	if (node->rgt) {
		if (!_avltree_walk(node->rgt, walker, arg))
			return false;
	}
	return true;
}

/** Walk the AVL tree in-order and apply the walker function on each visited
 * node.
 *
 * @param t		AVL tree to be walked.
 * @param walker	Walker function that will be called on each visited
 * 			node.
 * @param arg		Argument for the walker.
 */
void avltree_walk(avltree_t *t, avltree_walker_t walker, void *arg)
{
	if (t->root)
		_avltree_walk(t->root, walker, arg);
}

/** @}
 */

