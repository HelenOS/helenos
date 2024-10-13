/*
 * Copyright (c) 2024 Jiri Svoboda
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

#include <errno.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <str.h>
#include "../include/sif.h"

PCUT_INIT;

PCUT_TEST_SUITE(sif);
#if 0
/** Test sif_new and sif_delete. */
PCUT_TEST(sif_create)
{
	sif_doc_t *doc;
	errno_t rc;

	rc = sif_new(&doc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sif_delete(doc);
}

/** Test sif_load. */
PCUT_TEST(sif_load)
{
	sif_sess_t *sess;
	errno_t rc;
	int rv;
	char *fname;
	char *p;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_open(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_get_root. */
PCUT_TEST(sif_get_root)
{
	sif_sess_t *sess;
	sif_node_t *root;
	errno_t rc;
	int rv;
	char *fname;
	char *p;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);
	PCUT_ASSERT_NOT_NULL(root);
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(sif_node_get_type(root), "sif"));

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_node_prepend_child. */
PCUT_TEST(sif_node_prepend_child)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *ca;
	sif_node_t *cb;
	sif_node_t *c1;
	sif_node_t *c2;
	sif_node_t *c3;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_prepend_child(trans, root, "a", &ca);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_prepend_child(trans, root, "b", &cb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	c1 = sif_node_first_child(root);
	PCUT_ASSERT_TRUE(c1 == cb);

	c2 = sif_node_next_child(c1);
	PCUT_ASSERT_TRUE(c2 == ca);

	c3 = sif_node_next_child(c2);
	PCUT_ASSERT_TRUE(c3 == NULL);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_node_append_child. */
PCUT_TEST(sif_node_append_child)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *ca;
	sif_node_t *cb;
	sif_node_t *c1;
	sif_node_t *c2;
	sif_node_t *c3;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "a", &ca);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "b", &cb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	c1 = sif_node_first_child(root);
	PCUT_ASSERT_TRUE(c1 == ca);

	c2 = sif_node_next_child(c1);
	PCUT_ASSERT_TRUE(c2 == cb);

	c3 = sif_node_next_child(c2);
	PCUT_ASSERT_TRUE(c3 == NULL);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_node_insert_before. */
PCUT_TEST(sif_node_insert_before)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *ca;
	sif_node_t *cb;
	sif_node_t *cc;
	sif_node_t *c1;
	sif_node_t *c2;
	sif_node_t *c3;
	sif_node_t *c4;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "a", &ca);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "c", &cc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_insert_before(trans, cc, "b", &cb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	c1 = sif_node_first_child(root);
	PCUT_ASSERT_TRUE(c1 == ca);

	c2 = sif_node_next_child(c1);
	PCUT_ASSERT_TRUE(c2 == cb);

	c3 = sif_node_next_child(c2);
	PCUT_ASSERT_TRUE(c3 == cc);

	c4 = sif_node_next_child(c3);
	PCUT_ASSERT_TRUE(c4 == NULL);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_node_insert_after. */
PCUT_TEST(sif_node_insert_after)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *ca;
	sif_node_t *cb;
	sif_node_t *cc;
	sif_node_t *c1;
	sif_node_t *c2;
	sif_node_t *c3;
	sif_node_t *c4;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "a", &ca);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "c", &cc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_insert_after(trans, ca, "b", &cb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	c1 = sif_node_first_child(root);
	PCUT_ASSERT_TRUE(c1 == ca);

	c2 = sif_node_next_child(c1);
	PCUT_ASSERT_TRUE(c2 == cb);

	c3 = sif_node_next_child(c2);
	PCUT_ASSERT_TRUE(c3 == cc);

	c4 = sif_node_next_child(c3);
	PCUT_ASSERT_TRUE(c4 == NULL);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_node_destroy. */
PCUT_TEST(sif_node_destroy)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *ca;
	sif_node_t *cb;
	sif_node_t *cc;
	sif_node_t *c1;
	sif_node_t *c2;
	sif_node_t *c3;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "a", &ca);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "b", &cb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "c", &cc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sif_node_destroy(trans, cb);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	c1 = sif_node_first_child(root);
	PCUT_ASSERT_TRUE(c1 == ca);

	c2 = sif_node_next_child(c1);
	PCUT_ASSERT_TRUE(c2 == cc);

	c3 = sif_node_next_child(c2);
	PCUT_ASSERT_TRUE(c3 == NULL);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_node_set_attr. */
PCUT_TEST(sif_node_set_attr)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *node;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;
	const char *aval;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "node", &node);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_set_attr(trans, node, "a", "?");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_set_attr(trans, node, "a", "X");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_set_attr(trans, node, "b", "Y");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	aval = sif_node_get_attr(node, "a");
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(aval, "X"));

	aval = sif_node_get_attr(node, "b");
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(aval, "Y"));

	aval = sif_node_get_attr(node, "c");
	PCUT_ASSERT_NULL((void *) aval); // XXX PCUT_ASSERT_NULL does not accept const pointer

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test sif_node_unset_attr. */
PCUT_TEST(sif_node_unset_attr)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *node;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;
	const char *aval;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "node", &node);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_set_attr(trans, node, "a", "X");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	aval = sif_node_get_attr(node, "a");
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(aval, "X"));

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sif_node_unset_attr(trans, node, "a");
	sif_node_unset_attr(trans, node, "b");

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	aval = sif_node_get_attr(node, "a");
	PCUT_ASSERT_NULL((void *) aval); // XXX PCUT_ASSERT_NULL does not accept const pointer

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}

/** Test persistence of nodes and attributes. */
PCUT_TEST(sif_persist)
{
	sif_sess_t *sess;
	sif_node_t *root;
	sif_node_t *node;
	sif_trans_t *trans;
	errno_t rc;
	int rv;
	char *fname;
	char *p;
	const char *aval;

	fname = calloc(L_tmpnam, 1);
	PCUT_ASSERT_NOT_NULL(fname);

	p = tmpnam(fname);
	PCUT_ASSERT_TRUE(p == fname);

	rc = sif_create(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	rc = sif_trans_begin(sess, &trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_append_child(trans, root, "node", &node);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_node_set_attr(trans, node, "a", "X");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_trans_end(trans);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now reopen the repository */

	rc = sif_open(fname, &sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	root = sif_get_root(sess);

	node = sif_node_first_child(root);
	PCUT_ASSERT_NOT_NULL(node);
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(sif_node_get_type(node), "node"));

	aval = sif_node_get_attr(node, "a");
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(aval, "X"));

	rc = sif_close(sess);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
}
#endif
PCUT_EXPORT(sif);
