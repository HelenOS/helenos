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

#include <adt/odict.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <stdlib.h>

/** Test entry */
typedef struct {
	odlink_t odict;
	int key;
} test_entry_t;

enum {
	/** Length of test number sequences */
	test_seq_len = 100
};

/** Test get key function.
 *
 * @param odlink Ordered dictionary link
 * @return Pointer to key
 */
static void *test_getkey(odlink_t *odlink)
{
	return &odict_get_instance(odlink, test_entry_t, odict)->key;
}

/** Test compare function.
 *
 * @param a First key
 * @param b Second key
 * @return <0, 0, >0 if @a a is less than, equal or greater than @a b
 */
static int test_cmp(void *a, void *b)
{
	int *ia = (int *)a;
	int *ib = (int *)b;

	return *ia - *ib;
}

PCUT_INIT;

PCUT_TEST_SUITE(odict);

/** Increasing sequence test.
 *
 * Test initialization, emptiness, insertion of increasing sequence and walking.
 */
PCUT_TEST(incr_seq)
{
	odict_t odict;
	test_entry_t *e;
	odlink_t *c;
	int i;

	odict_initialize(&odict, test_getkey, test_cmp);

	PCUT_ASSERT_EQUALS(true, odict_empty(&odict));

	for (i = 0; i < test_seq_len; i++) {
		e = calloc(1, sizeof(test_entry_t));
		PCUT_ASSERT_NOT_NULL(e);

		e->key = i;
		odict_insert(&e->odict, &odict, NULL);
		PCUT_ASSERT_ERRNO_VAL(EOK, odict_validate(&odict));
	}

	i = 0;
	c = odict_first(&odict);
	while (c != NULL) {
		e = odict_get_instance(c, test_entry_t, odict);
		PCUT_ASSERT_INT_EQUALS(i, e->key);
		c = odict_next(c, &odict);
		++i;
	}
	PCUT_ASSERT_INT_EQUALS(test_seq_len, i);

	i = test_seq_len;
	c = odict_last(&odict);
	while (c != NULL) {
		--i;
		e = odict_get_instance(c, test_entry_t, odict);
		PCUT_ASSERT_INT_EQUALS(i, e->key);
		c = odict_prev(c, &odict);
	}

	PCUT_ASSERT_INT_EQUALS(0, i);
}

/** Decreasing sequence test.
 *
 * Test initialization, emptiness, insertion of decreasing sequence and walking.
 */
PCUT_TEST(decr_seq)
{
	odict_t odict;
	test_entry_t *e;
	odlink_t *c;
	int i;

	odict_initialize(&odict, test_getkey, test_cmp);

	PCUT_ASSERT_EQUALS(true, odict_empty(&odict));

	for (i = 0; i < test_seq_len; i++) {
		e = calloc(1, sizeof(test_entry_t));
		PCUT_ASSERT_NOT_NULL(e);

		e->key = test_seq_len - i - 1;
		odict_insert(&e->odict, &odict, NULL);
		PCUT_ASSERT_ERRNO_VAL(EOK, odict_validate(&odict));
	}

	i = 0;
	c = odict_first(&odict);
	while (c != NULL) {
		e = odict_get_instance(c, test_entry_t, odict);
		PCUT_ASSERT_INT_EQUALS(i, e->key);
		c = odict_next(c, &odict);
		++i;
	}
	PCUT_ASSERT_INT_EQUALS(test_seq_len, i);

	i = test_seq_len;
	c = odict_last(&odict);
	while (c != NULL) {
		--i;
		e = odict_get_instance(c, test_entry_t, odict);
		PCUT_ASSERT_INT_EQUALS(i, e->key);
		c = odict_prev(c, &odict);
	}

	PCUT_ASSERT_INT_EQUALS(0, i);
}

/** Increasing sequence insertion and removal test.
 *
 * Test sequential insertion of increasing sequence and sequential removal.
 */
PCUT_TEST(incr_seq_ins_rem)
{
	odict_t odict;
	test_entry_t *e;
	odlink_t *c;
	int i;

	odict_initialize(&odict, test_getkey, test_cmp);

	PCUT_ASSERT_EQUALS(true, odict_empty(&odict));

	for (i = 0; i < test_seq_len; i++) {
		e = calloc(1, sizeof(test_entry_t));
		PCUT_ASSERT_NOT_NULL(e);

		e->key = i;
		odict_insert(&e->odict, &odict, NULL);
		PCUT_ASSERT_ERRNO_VAL(EOK, odict_validate(&odict));
	}

	i = 0;
	c = odict_first(&odict);
	while (c != NULL) {
		e = odict_get_instance(c, test_entry_t, odict);
		PCUT_ASSERT_INT_EQUALS(i, e->key);

		odict_remove(c);
		PCUT_ASSERT_ERRNO_VAL(EOK, odict_validate(&odict));

		c = odict_first(&odict);
		++i;
	}

	PCUT_ASSERT_INT_EQUALS(test_seq_len, i);
}

/** Generate pseudorandom sequence. */
static int seq_next(int cur)
{
	return (cur * 1951) % 1000000;
}

/** Test 4.
 *
 * Test inserting a pseudorandom sequence and then extracting it again.
 */
PCUT_TEST(prseq_ins_extract)
{
	odict_t odict;
	test_entry_t *e, *ep;
	odlink_t *c, *d;
	int prev;
	int i;
	int v;

	odict_initialize(&odict, test_getkey, test_cmp);

	PCUT_ASSERT_EQUALS(true, odict_empty(&odict));

	v = 1;
	ep = NULL;
	for (i = 0; i < test_seq_len; i++) {
		e = calloc(1, sizeof(test_entry_t));
		PCUT_ASSERT_NOT_NULL(e);

		e->key = v;
		odict_insert(&e->odict, &odict, &ep->odict);
		PCUT_ASSERT_ERRNO_VAL(EOK, odict_validate(&odict));
		v = seq_next(v);
		ep = e;
	}

	/* Verify that entries are in ascending order */
	c = odict_first(&odict);
	prev = -1;
	i = 0;
	while (c != NULL) {
		e = odict_get_instance(c, test_entry_t, odict);
		PCUT_ASSERT_EQUALS(true, e->key > prev);

		prev = e->key;
		c = odict_next(c, &odict);
		++i;
	}

	PCUT_ASSERT_INT_EQUALS(test_seq_len, i);

	/* Try extracting the sequence */
	v = 1;
	for (i = 0; i < test_seq_len; i++) {
		c = odict_find_eq(&odict, (void *)&v, NULL);
		PCUT_ASSERT_NOT_NULL(c);

		e = odict_get_instance(c, test_entry_t, odict);
		PCUT_ASSERT_INT_EQUALS(v, e->key);

		d = odict_find_eq_last(&odict, (void *)&v, NULL);
		PCUT_ASSERT_NOT_NULL(d);

		e = odict_get_instance(d, test_entry_t, odict);
		PCUT_ASSERT_INT_EQUALS(v, e->key);

		odict_remove(c);
		PCUT_ASSERT_ERRNO_VAL(EOK, odict_validate(&odict));

		v = seq_next(v);
	}
}

PCUT_EXPORT(odict);
