/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */

/** @file Ordered dictionary.
 *
 * Implementation based on red-black trees.
 * Note that non-data ('leaf') nodes are implemented as NULLs, not
 * as actual nodes.
 */

#include <adt/list.h>
#include <adt/odict.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void odict_pgu(odlink_t *, odlink_t **, odict_child_sel_t *,
    odlink_t **, odict_child_sel_t *, odlink_t **);

static void odict_rotate_left(odlink_t *);
static void odict_rotate_right(odlink_t *);
static void odict_swap_node(odlink_t *, odlink_t *);
static void odict_replace_subtree(odlink_t *, odlink_t *);
static void odict_unlink(odlink_t *);
static void odict_link_child_a(odlink_t *, odlink_t *);
static void odict_link_child_b(odlink_t *, odlink_t *);
static void odict_sibling(odlink_t *, odlink_t *, odict_child_sel_t *,
    odlink_t **);
static odlink_t *odict_search_start_node(odict_t *, void *, odlink_t *);

/** Print subtree.
 *
 * Print subtree rooted at @a cur
 *
 * @param cur Root of tree to print
 */
static void odict_print_tree(odlink_t *cur)
{
	if (cur == NULL) {
		printf("0");
		return;
	}

	printf("[%p/%c", cur, cur->color == odc_red ? 'r' : 'b');
	if (cur->a != NULL || cur->b != NULL) {
		putchar(' ' );
		odict_print_tree(cur->a);
		putchar(',');
		odict_print_tree(cur->b);
	}
	putchar(']');
}

/** Validate ordered dictionary subtree.
 *
 * Verify that red-black tree properties are satisfied.
 *
 * @param cur Root of tree to verify
 * @param rbd Place to store black depth of the subtree
 *
 * @return EOK on success, EINVAL on failure
 */
static errno_t odict_validate_tree(odlink_t *cur, int *rbd)
{
	errno_t rc;
	int bd_a, bd_b;
	int cur_d;

	if (cur->up == NULL) {
		/* Verify root pointer */
		if (cur->odict->root != cur) {
			printf("cur->up == NULL and yet cur != root\n");
			return EINVAL;
		}

		/* Verify root color */
		if (cur->color != odc_black) {
			printf("Root is not black\n");
			return EINVAL;
		}
	}

	if (cur->a != NULL) {
		/* Verify symmetry of a - up links */
		if (cur->a->up != cur) {
			printf("cur->a->up != cur\n");
			return EINVAL;
		}

		/* Verify that if a node is red, its left child is red */
		if (cur->a->color == odc_red && cur->color == odc_red) {
			printf("cur->a is red, cur is red\n");
			return EINVAL;
		}

		/* Recurse to left child */
		rc = odict_validate_tree(cur->a, &bd_a);
		if (rc != EOK)
			return rc;
	} else {
		bd_a = -1;
	}

	if (cur->b != NULL) {
		/* Verify symmetry of b - up links */
		if (cur->b->up != cur) {
			printf("cur->b->up != cur\n");
			return EINVAL;
		}

		/* Verify that if a node is red, its right child is red */
		if (cur->b->color == odc_red && cur->color == odc_red) {
			printf("cur->b is red, cur is red\n");
			return EINVAL;
		}

		/* Recurse to right child */
		rc = odict_validate_tree(cur->b, &bd_b);
		if (rc != EOK)
			return rc;
	} else {
		bd_b = -1;
	}

	/* Verify that black depths of both children are equal */
	if (bd_a >= 0 && bd_b >= 0) {
		if (bd_a != bd_b) {
			printf("Black depth %d != %d\n", bd_a, bd_b);
			return EINVAL;
		}
	}

	cur_d = cur->color == odc_black ? 1 : 0;
	if (bd_a >= 0)
		*rbd = bd_a + cur_d;
	else if (bd_b >= 0)
		*rbd = bd_b + cur_d;
	else
		*rbd = cur_d;

	return EOK;
}

/** Validate ordered dictionary properties.
 *
 * @param odict Ordered dictionary
 */
errno_t odict_validate(odict_t *odict)
{
	int bd;
	errno_t rc;

	if (odict->root == NULL)
		return EOK;

	rc = odict_validate_tree(odict->root, &bd);
	if (rc != EOK)
		odict_print_tree(odict->root);

	return rc;
}

/** Initialize ordered dictionary.
 *
 * @param odict Ordered dictionary
 * @param getkey Funcition to get key
 * @param cmp Function to compare entries
 */
void odict_initialize(odict_t *odict, odgetkey_t getkey, odcmp_t cmp)
{
	odict->root = NULL;
	list_initialize(&odict->entries);
	odict->getkey = getkey;
	odict->cmp = cmp;
}

/** Initialize ordered dictionary link.
 *
 * @param odlink Ordered dictionary link
 */
void odlink_initialize(odlink_t *odlink)
{
	odlink->odict = NULL;
	odlink->up = NULL;
	odlink->a = NULL;
	odlink->b = NULL;
	link_initialize(&odlink->lentries);
}

/** Insert entry in ordered dictionary.
 *
 * Insert entry in ordered dictionary, placing it after other entries
 * with the same key.
 *
 * @param odlink New entry
 * @param odict Ordered dictionary
 * @param hint An entry that might be near the new entry or @c NULL
 */
void odict_insert(odlink_t *odlink, odict_t *odict, odlink_t *hint)
{
	int d;
	odlink_t *cur;
	odlink_t *p;
	odlink_t *g;
	odlink_t *u;
	odict_child_sel_t pcs, gcs;

	assert(!odlink_used(odlink));

	if (odict->root == NULL) {
		/* odlink is the root node */
		odict->root = odlink;
		odlink->odict = odict;
		odlink->color = odc_black;
		list_append(&odlink->lentries, &odict->entries);
		return;
	}

	cur = odict_search_start_node(odict, odict->getkey(odlink), hint);
	while (true) {
		d = odict->cmp(odict->getkey(odlink), odict->getkey(cur));
		if (d < 0) {
			if (cur->a == NULL) {
				odict_link_child_a(odlink, cur);
	    			break;
			}
			cur = cur->a;
		} else {
			if (cur->b == NULL) {
				odict_link_child_b(odlink, cur);
				break;
			}
			cur = cur->b;
		}
	}


	odlink->color = odc_red;

	while (true) {
		/* Fix up odlink and its parent potentially being red */
		if (odlink->up == NULL) {
			odlink->color = odc_black;
			break;
		}

		if (odlink->up->color == odc_black)
			break;

		/* Get parent, grandparent, uncle */
		odict_pgu(odlink, &p, &pcs, &g, &gcs, &u);

		if (g == NULL) {
			p->color = odc_black;
			break;
		}

		if (p->color == odc_red && u != NULL && u->color == odc_red) {
			/* Parent and uncle are both red */
			p->color = odc_black;
			u->color = odc_black;
			g->color = odc_red;
			odlink = g;
			continue;
		}

		/* Parent is red but uncle is black, odlink-P-G is trans */
		if (pcs != gcs) {
			if (gcs == odcs_a) {
				/* odlink is right child of P */
				/* P is left child of G */
				odict_rotate_left(p);
			} else {
				/* odlink is left child of P */
				/* P is right child of G */
				odict_rotate_right(p);
			}

			odlink = p;
			odict_pgu(odlink, &p, &pcs, &g, &gcs, &u);
		}

		/* odlink-P-G is now cis */
		assert(pcs == gcs);
		if (pcs == odcs_a) {
			/* odlink is left child of P */
			/* P is left child of G */
			odict_rotate_right(g);
		} else {
			/* odlink is right child of P */
			/* P is right child of G */
			odict_rotate_left(g);
		}

		p->color = odc_black;
		g->color = odc_red;
		break;
	}
}

/** Remove entry from ordered dictionary.
 *
 * @param odlink Ordered dictionary link
 */
void odict_remove(odlink_t *odlink)
{
	odlink_t *n;
	odlink_t *c;
	odlink_t *p;
	odlink_t *s;
	odlink_t *sc, *st;
	odict_child_sel_t pcs;

	if (odlink->a != NULL && odlink->b != NULL) {
		n = odict_next(odlink, odlink->odict);
		assert(n != NULL);

		odict_swap_node(odlink, n);
	}

	/* odlink has at most one child */
	if (odlink->a != NULL) {
		assert(odlink->b == NULL);
		c = odlink->a;
	} else {
		c = odlink->b;
	}

	if (odlink->color == odc_red) {
		/* We can remove it harmlessly */
		assert(c == NULL);
		odict_unlink(odlink);
		return;
	}

	/* odlink->color == odc_black */
	if (c != NULL && c->color == odc_red) {
		/* Child is red: swap colors of S and C */
		c->color = odc_black;
		odict_replace_subtree(c, odlink);
		odlink->up = odlink->a = odlink->b = NULL;
		odlink->odict = NULL;
		list_remove(&odlink->lentries);
		return;
	}

	/* There cannot be one black child */
	assert(c == NULL);

	n = NULL;
	p = odlink->up;
	odict_unlink(odlink);
	/* We removed one black node, creating imbalance */
again:
	/* Case 1: N is the new root */
	if (p == NULL)
		return;

	odict_sibling(n, p, &pcs, &s);

	/* Paths through N have one less black node than paths through S */

	/* Case 2: S is red */
	if (s->color == odc_red) {
		assert(p->color == odc_black);
		p->color = odc_red;
		s->color = odc_black;
		if (n == p->a)
			odict_rotate_left(p);
		else
			odict_rotate_right(p);
		odict_sibling(n, p, &pcs, &s);
		/* Now S is black */
		assert(s->color == odc_black);
	}

	/* Case 3: P, S and S's children are black */
	if (p->color == odc_black &&
	    s->color == odc_black &&
	    (s->a == NULL || s->a->color == odc_black) &&
	    (s->b == NULL || s->b->color == odc_black)) {
		/*
		 * Changing S to red means all paths through S or N have one
		 * less black node than they should. So redo the same for P.
		 */
		s->color = odc_red;
		n = p;
		p = n->up;
		goto again;
	}

	/* Case 4: P is red, S and S's children are black */
	if (p->color == odc_red &&
	    s->color == odc_black &&
	    (s->a == NULL || s->a->color == odc_black) &&
	    (s->b == NULL || s->b->color == odc_black)) {
		/* Swap colors of S and P */
		s->color = odc_red;
		p->color = odc_black;
		return;
	}

	/* N is the left child */
	if (pcs == odcs_a) {
		st = s->a;
		sc = s->b;
	} else {
		st = s->b;
		sc = s->a;
	}

	/* Case 5: S is black and S's trans child is red, S's cis child is black */
	if (s->color == odc_black &&
	    (st != NULL && st->color == odc_red) &&
	    (sc == NULL || sc->color == odc_black)) {
		/* N is the left child */
		if (pcs == odcs_a)
			odict_rotate_right(s);
		else
			odict_rotate_left(s);
		s->color = odc_red;
		s->up->color = odc_black;
		/* Now N has a black sibling whose cis child is red */
		odict_sibling(n, p, &pcs, &s);
		/* N is the left child */
		if (pcs == odcs_a) {
			st = s->a;
			sc = s->b;
		} else {
			st = s->b;
			sc = s->a;
		}
	}

	/* Case 6: S is black, S's cis child is red */
	assert(s->color == odc_black);
	assert(sc != NULL);
	assert(sc->color == odc_red);

	if (pcs == odcs_a)
		odict_rotate_left(p);
	else
		odict_rotate_right(p);

	s->color = p->color;
	p->color = odc_black;
	sc->color = odc_black;
}

/** Update dictionary after entry key has been changed.
 *
 * After the caller modifies the key of an entry, they need to call
 * this function so that the dictionary can update itself accordingly.
 *
 * @param odlink Ordered dictionary entry
 * @param odict Ordered dictionary
 */
void odict_key_update(odlink_t *odlink, odict_t *odict)
{
	odlink_t *n;

	n = odict_next(odlink, odict);
	odict_remove(odlink);
	odict_insert(odlink, odict, n);
}

/** Return true if entry is in a dictionary.
 *
 * @param odlink Ordered dictionary entry
 * @return @c true if entry is in a dictionary, @c false otherwise
 */
bool odlink_used(odlink_t *odlink)
{
	return odlink->odict != NULL;
}

/** Return true if ordered dictionary is empty.
 *
 * @param odict Ordered dictionary
 * @return @c true if @a odict is emptry, @c false otherwise
 */
bool odict_empty(odict_t *odict)
{
	return odict->root == NULL;
}

/** Return the number of entries in @a odict.
 *
 * @param odict Ordered dictionary
 */
unsigned long odict_count(odict_t *odict)
{
	unsigned long cnt;
	odlink_t *cur;

	cnt = 0;
	cur = odict_first(odict);
	while (cur != NULL) {
		++cnt;
		cur = odict_next(cur, odict);
	}

	return cnt;
}

/** Return first entry in a list or @c NULL if list is empty.
 *
 * @param odict Ordered dictionary
 * @return First entry
 */
odlink_t *odict_first(odict_t *odict)
{
	link_t *link;

	link = list_first(&odict->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, odlink_t, lentries);
}

/** Return last entry in a list or @c NULL if list is empty
 *
 * @param odict Ordered dictionary
 * @return Last entry
 */
odlink_t *odict_last(odict_t *odict)
{
	link_t *link;

	link = list_last(&odict->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, odlink_t, lentries);
}

/** Return previous entry in list or @c NULL if @a link is the first one.
 *
 * @param odlink Entry
 * @param odict Ordered dictionary
 * @return Previous entry
 */
odlink_t *odict_prev(odlink_t *odlink, odict_t *odict)
{
	link_t *link;

	link = list_prev(&odlink->lentries, &odlink->odict->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, odlink_t, lentries);
}

/** Return next entry in dictionary or @c NULL if @a odlink is the last one
 *
 * @param odlink Entry
 * @param odict Ordered dictionary
 * @return Next entry
 */
odlink_t *odict_next(odlink_t *odlink, odict_t *odict)
{
	link_t *link;

	link = list_next(&odlink->lentries, &odlink->odict->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, odlink_t, lentries);
}

/** Find first entry whose key is equal to @a key/
 *
 * @param odict Ordered dictionary
 * @param key Key
 * @param hint Nearby entry
 * @return Pointer to entry on success, @c NULL on failure
 */
odlink_t *odict_find_eq(odict_t *odict, void *key, odlink_t *hint)
{
	odlink_t *geq;

	geq = odict_find_geq(odict, key, hint);
	if (geq == NULL)
		return NULL;

	if (odict->cmp(odict->getkey(geq), key) == 0)
		return geq;
	else
		return NULL;
}

/** Find last entry whose key is equal to @a key/
 *
 * @param odict Ordered dictionary
 * @param key Key
 * @param hint Nearby entry
 * @return Pointer to entry on success, @c NULL on failure
 */
odlink_t *odict_find_eq_last(odict_t *odict, void *key, odlink_t *hint)
{
	odlink_t *leq;

	leq = odict_find_leq(odict, key, hint);
	if (leq == NULL)
		return NULL;

	if (odict->cmp(odict->getkey(leq), key) == 0)
		return leq;
	else
		return NULL;
}

/** Find first entry whose key is greater than or equal to @a key
 *
 * @param odict Ordered dictionary
 * @param key Key
 * @param hint Nearby entry
 * @return Pointer to entry on success, @c NULL on failure
 */
odlink_t *odict_find_geq(odict_t *odict, void *key, odlink_t *hint)
{
	odlink_t *cur;
	odlink_t *next;
	int d;

	cur = odict_search_start_node(odict, key, hint);
	if (cur == NULL)
		return NULL;

	while (true) {
		d = odict->cmp(odict->getkey(cur), key);
		if (d >= 0)
			next = cur->a;
		else
			next = cur->b;

		if (next == NULL)
			break;

		cur = next;
	}

	if (d >= 0) {
		return cur;
	} else {
		return odict_next(cur, odict);
	}
}

/** Find last entry whose key is greater than @a key.
 *
 * @param odict Ordered dictionary
 * @param key Key
 * @param hint Nearby entry
 * @return Pointer to entry on success, @c NULL on failure
 */
odlink_t *odict_find_gt(odict_t *odict, void *key, odlink_t *hint)
{
	odlink_t *leq;

	leq = odict_find_leq(odict, key, hint);
	if (leq != NULL)
		return odict_next(leq, odict);
	else
		return odict_first(odict);
}

/** Find last entry whose key is less than or equal to @a key
 *
 * @param odict Ordered dictionary
 * @param key Key
 * @param hint Nearby entry
 * @return Pointer to entry on success, @c NULL on failure
 */
odlink_t *odict_find_leq(odict_t *odict, void *key, odlink_t *hint)
{
	odlink_t *cur;
	odlink_t *next;
	int d;

	cur = odict_search_start_node(odict, key, hint);
	if (cur == NULL)
		return NULL;

	while (true) {
		d = odict->cmp(key, odict->getkey(cur));
		if (d >= 0)
			next = cur->b;
		else
			next = cur->a;

		if (next == NULL)
			break;

		cur = next;
	}

	if (d >= 0) {
		return cur;
	} else {
		return odict_prev(cur, odict);
	}
}

/** Find last entry whose key is less than @a key.
 *
 * @param odict Ordered dictionary
 * @param key Key
 * @param hint Nearby entry
 * @return Pointer to entry on success, @c NULL on failure
 */
odlink_t *odict_find_lt(odict_t *odict, void *key, odlink_t *hint)
{
	odlink_t *geq;

	geq = odict_find_geq(odict, key, hint);
	if (geq != NULL)
		return odict_prev(geq, odict);
	else
		return odict_last(odict);
}

/** Return parent, grandparent and uncle.
 *
 * @param n Node
 * @param p Place to store pointer to parent of @a n
 * @param pcs Place to store position of @a n w.r.t. @a p
 * @param g Place to store pointer to grandparent of @a n
 * @param gcs Place to store position of @a p w.r.t. @a g
 * @param u Place to store pointer to uncle of @a n
 */
static void odict_pgu(odlink_t *n, odlink_t **p, odict_child_sel_t *pcs,
    odlink_t **g, odict_child_sel_t *gcs, odlink_t **u)
{
	*p = n->up;

	if (*p == NULL) {
		/* No parent */
		*g = NULL;
		*u = NULL;
		return;
	}

	if ((*p)->a == n) {
		*pcs = odcs_a;
	} else {
		assert((*p)->b == n);
		*pcs = odcs_b;
	}

	*g = (*p)->up;
	if (*g == NULL) {
		/* No grandparent */
		*u = NULL;
		return;
	}

	if ((*g)->a == *p) {
		*gcs = odcs_a;
		*u = (*g)->b;
	} else {
		assert((*g)->b == *p);
		*gcs = odcs_b;
		*u = (*g)->a;
	}
}

/** Return sibling and parent w.r.t. parent.
 *
 * @param n Node
 * @param p Parent of @ an
 * @param pcs Place to store position of @a n w.r.t. @a p.
 * @param rs Place to strore pointer to sibling
 */
static void odict_sibling(odlink_t *n, odlink_t *p, odict_child_sel_t *pcs,
    odlink_t **rs)
{
	if (p->a == n) {
		*pcs = odcs_a;
		*rs = p->b;
	} else {
		*pcs = odcs_b;
		*rs = p->a;
	}
}

/** Ordered dictionary left rotation.
 *
 *    Q           P
 *  P   C   <- A    Q
 * A B             B C
 *
 */
static void odict_rotate_left(odlink_t *p)
{
	odlink_t *q;

	q = p->b;
	assert(q != NULL);

	/* Replace P with Q as the root of the subtree */
	odict_replace_subtree(q, p);

	/* Relink P under Q, B under P */
	p->up = q;
	p->b = q->a;
	if (p->b != NULL)
		p->b->up = p;
	q->a = p;

	/* Fix odict root */
	if (p->odict->root == p)
		p->odict->root = q;
}

/** Ordered dictionary right rotation.
 *
 *    Q           P
 *  P   C   -> A    Q
 * A B             B C
 *
 */
static void odict_rotate_right(odlink_t *q)
{
	odlink_t *p;

	p = q->a;
	assert(p != NULL);

	/* Replace Q with P as the root of the subtree */
	odict_replace_subtree(p, q);

	/* Relink Q under P, B under Q */
	q->up = p;
	q->a = p->b;
	if (q->a != NULL)
		q->a->up = q;
	p->b = q;

	/* Fix odict root */
	if (q->odict->root == q)
		q->odict->root = p;
}

/** Swap two nodes.
 *
 * Swap position of two nodes in the tree, keeping their identity.
 * This means we don't copy the contents, instead we shuffle around pointers
 * from and to the nodes.
 *
 * @param a First node
 * @param b Second node
 */
static void odict_swap_node(odlink_t *a, odlink_t *b)
{
	odlink_t *n;
	odict_color_t c;

	/* Backlink from A's parent */
	if (a->up != NULL && a->up != b) {
		if (a->up->a == a) {
			a->up->a = b;
		} else {
			assert(a->up->b == a);
			a->up->b = b;
		}
	}

	/* Backlink from A's left child */
	if (a->a != NULL && a->a != b)
		a->a->up = b;
	/* Backling from A's right child */
	if (a->b != NULL && a->b != b)
		a->b->up = b;

	/* Backlink from B's parent */
	if (b->up != NULL && b->up != a) {
		if (b->up->a == b) {
			b->up->a = a;
		} else {
			assert(b->up->b == b);
			b->up->b = a;
		}
	}

	/* Backlink from B's left child */
	if (b->a != NULL && b->a != a)
		b->a->up = a;
	/* Backling from B's right child */
	if (b->b != NULL && b->b != a)
		b->b->up = a;

	/* Swap links going out of A and out of B */
	n = a->up; a->up = b->up; b->up = n;
	n = a->a; a->a = b->a; b->a = n;
	n = a->b; a->b = b->b; b->b = n;
	c = a->color; a->color = b->color; b->color = c;

	/* When A and B are adjacent, fix self-loops that might have arisen */
	if (a->up == a)
		a->up = b;
	if (a->a == a)
		a->a = b;
	if (a->b == a)
		a->b = b;
	if (b->up == b)
		b->up = a;
	if (b->a == b)
		b->a = a;
	if (b->b == b)
		b->b = a;

	/* Fix odict root */
	if (a == a->odict->root)
		a->odict->root = b;
	else if (b == a->odict->root)
		a->odict->root = a;
}

/** Replace subtree.
 *
 * Replace subtree @a old with another subtree @a n. This makes the parent
 * point to the new subtree root and the up pointer of @a n to point to
 * the parent.
 *
 * @param old Subtree to be replaced
 * @param n New subtree
 */
static void odict_replace_subtree(odlink_t *n, odlink_t *old)
{
	if (old->up != NULL) {
		if (old->up->a == old) {
			old->up->a = n;
		} else {
			assert(old->up->b == old);
			old->up->b = n;
		}
	} else {
		assert(old->odict->root == old);
		old->odict->root = n;
	}

	n->up = old->up;
}

/** Unlink node.
 *
 * @param n Ordered dictionary node
 */
static void odict_unlink(odlink_t *n)
{
	if (n->up != NULL) {
		if (n->up->a == n) {
			n->up->a = NULL;
		} else {
			assert(n->up->b == n);
			n->up->b = NULL;
		}

		n->up = NULL;
	} else {
		assert(n->odict->root == n);
		n->odict->root = NULL;
	}

	if (n->a != NULL) {
		n->a->up = NULL;
		n->a = NULL;
	}

	if (n->b != NULL) {
		n->b->up = NULL;
		n->b = NULL;
	}

	n->odict = NULL;
	list_remove(&n->lentries);
}

/** Link node as left child.
 *
 * Append new node @a n as left child of existing node @a old.
 *
 * @param n New node
 * @param old Old node
 */
static void odict_link_child_a(odlink_t *n, odlink_t *old)
{
	old->a = n;
	n->up = old;
	n->odict = old->odict;
	list_insert_before(&n->lentries, &old->lentries);
}

/** Link node as right child.
 *
 * Append new node @a n as right child of existing node @a old.
 *
 * @param n New node
 * @param old Old node
 */
static void odict_link_child_b(odlink_t *n, odlink_t *old)
{
	old->b = n;
	n->up = old;
	n->odict = old->odict;
	list_insert_after(&n->lentries, &old->lentries);
}

/** Get node where search should be started.
 *
 * @param odict Ordered dictionary
 * @param key Key being searched for
 * @param hint Node that might be near the search target or @c NULL
 *
 * @return Node from where search should be started
 */
static odlink_t *odict_search_start_node(odict_t *odict, void *key,
    odlink_t *hint)
{
	odlink_t *a;
	odlink_t *b;
	odlink_t *cur;
	int d, da, db;

	assert(hint == NULL || hint->odict == odict);

	/* If the key is greater than the maximum, start search in the maximum */
	b = odict_last(odict);
	if (b != NULL) {
		d = odict->cmp(odict->getkey(b), key);
		if (d < 0)
			return b;
	}

	/* If the key is less tna the minimum, start search in the minimum */
	a = odict_first(odict);
	if (a != NULL) {
		d = odict->cmp(key, odict->getkey(a));
		if (d < 0)
			return a;
	}

	/*
	 * Proposition: Let A, B be two BST nodes such that B is a descendant
	 * of A. Let N be a node such that either key(A) < key(N) < key(B)
	 * Then N is a descendant of A.
	 * Corollary: We can start searching for N from A, instead from
	 * the root.
	 *
	 * Proof: When walking the BST in order, visit_tree(A) does a
	 * visit_tree(A->a), visit(A), visit(A->b). If key(A) < key(B),
	 * we will first visit A, then while visiting all nodes with values
	 * between A and B we will not leave subtree A->b.
	 */

	/* If there is no hint, start search from the root */
	if (hint == NULL)
		return odict->root;

	/*
	 * Start from hint and walk up to the root, keeping track of
	 * minimum and maximum. Once key is strictly between them,
	 * we can return the current node, which we've proven to be
	 * an ancestor of a potential node with the given key
	 */
	a = b = cur = hint;
	while (cur->up != NULL) {
		cur = cur->up;

		d = odict->cmp(odict->getkey(cur), odict->getkey(a));
		if (d < 0)
			a = cur;

		d = odict->cmp(odict->getkey(b), odict->getkey(cur));
		if (d < 0)
			b = cur;

		da = odict->cmp(odict->getkey(a), key);
		db = odict->cmp(key, odict->getkey(b));
		if (da < 0 && db < 0) {
			/* Both a and b are descendants of cur */
			return cur;
		}
	}

	return odict->root;
}

/** @}
 */
