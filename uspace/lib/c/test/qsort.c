/*
 * Copyright (c) 2017 Jiri Svoboda
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

#include <pcut/pcut.h>
#include <qsort.h>
#include <stdio.h>
#include <stdlib.h>

enum {
	/** Length of test number sequences */
	test_seq_len = 5
};

/** Test compare function.
 *
 * @param a First key
 * @param b Second key
 * @return <0, 0, >0 if @a a is less than, equal or greater than @a b
 */
static int test_cmp(const void *a, const void *b)
{
	int *ia = (int *)a;
	int *ib = (int *)b;

	return *ia - *ib;
}

static void bubble_sort(int *seq, size_t nmemb)
{
	size_t i;
	size_t j;
	int t;

	for (i = 0; i < nmemb - 1; i++)
		for (j = 0; j < nmemb - 1; j++) {
			if (seq[j] > seq[j + 1]) {
				t = seq[j];
				seq[j] = seq[j + 1];
				seq[j + 1] = t;
			}
		}
}

PCUT_INIT;

PCUT_TEST_SUITE(qsort);

/** Test sorting already sorted increasing sequence */
PCUT_TEST(incr_seq)
{
	int *seq;
	int i;

	seq = calloc(test_seq_len, sizeof(int));
	PCUT_ASSERT_NOT_NULL(seq);

	for (i = 0; i < test_seq_len; i++)
		seq[i] = i;

	qsort(seq, test_seq_len, sizeof(int), test_cmp);

	for (i = 0; i < test_seq_len; i++) {
		PCUT_ASSERT_INT_EQUALS(i, seq[i]);
	}

	free(seq);
}

/** Test sorting reverse-sorted decreasing sequence. */
PCUT_TEST(decr_seq)
{
	int *seq;
	int i;

	seq = calloc(test_seq_len, sizeof(int));
	PCUT_ASSERT_NOT_NULL(seq);

	for (i = 0; i < test_seq_len; i++)
		seq[i] = test_seq_len - 1 - i;

	qsort(seq, test_seq_len, sizeof(int), test_cmp);

	for (i = 0; i < test_seq_len; i++) {
		PCUT_ASSERT_INT_EQUALS(i, seq[i]);
	}

	free(seq);
}

/** Test sorting reverse-sorted decreasing sequence where each member repeats twice. */
PCUT_TEST(decr2_seq)
{
	int *seq;
	int i;

	seq = calloc(test_seq_len, sizeof(int));
	PCUT_ASSERT_NOT_NULL(seq);

	for (i = 0; i < test_seq_len; i++) {
		seq[i] = (test_seq_len - 1 - i) / 2;
	}

	qsort(seq, test_seq_len, sizeof(int), test_cmp);

	for (i = 0; i < test_seq_len; i++) {
		PCUT_ASSERT_INT_EQUALS(i / 2, seq[i]);
	}

	free(seq);
}

/** Generate pseudorandom sequence. */
static int seq_next(int cur)
{
	return (cur * 1951) % 1000000;
}

/** Test sorting pseudorandom sequence. */
PCUT_TEST(pseudorandom_seq)
{
	int *seq, *seq2;
	int i;
	int v;

	seq = calloc(test_seq_len, sizeof(int));
	PCUT_ASSERT_NOT_NULL(seq);

	seq2 = calloc(test_seq_len, sizeof(int));
	PCUT_ASSERT_NOT_NULL(seq);

	v = 1;
	for (i = 0; i < test_seq_len; i++) {
		seq[i] = v;
		seq2[i] = v;
		v = seq_next(v);
	}

	qsort(seq, test_seq_len, sizeof(int), test_cmp);
	bubble_sort(seq2, test_seq_len);

	for (i = 0; i < test_seq_len; i++) {
		PCUT_ASSERT_INT_EQUALS(seq2[i], seq[i]);
	}

	free(seq);
	free(seq2);
}

PCUT_EXPORT(qsort);
