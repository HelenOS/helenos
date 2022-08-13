/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <adt/circ_buf.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(circ_buf);

enum {
	buffer_size = 16
};

static int buffer[buffer_size];

/** Basic insertion/deletion test.
 *
 * Test initialization, emptiness, pushing buffer until it is full,
 * then emptying it again.
 */
PCUT_TEST(push_pop)
{
	circ_buf_t cbuf;
	int i;
	int j;
	errno_t rc;

	circ_buf_init(&cbuf, buffer, buffer_size, sizeof(int));

	for (i = 0; i < buffer_size; i++) {
		PCUT_ASSERT_INT_EQUALS(buffer_size - i, circ_buf_nfree(&cbuf));
		PCUT_ASSERT_INT_EQUALS(i, circ_buf_nused(&cbuf));
		rc = circ_buf_push(&cbuf, &i);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	}

	rc = circ_buf_push(&cbuf, &i);
	PCUT_ASSERT_ERRNO_VAL(EAGAIN, rc);

	for (i = 0; i < buffer_size; i++) {
		PCUT_ASSERT_INT_EQUALS(i, circ_buf_nfree(&cbuf));
		PCUT_ASSERT_INT_EQUALS(buffer_size - i, circ_buf_nused(&cbuf));
		rc = circ_buf_pop(&cbuf, &j);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_INT_EQUALS(i, j);
	}

	PCUT_ASSERT_INT_EQUALS(buffer_size, circ_buf_nfree(&cbuf));
	PCUT_ASSERT_INT_EQUALS(0, circ_buf_nused(&cbuf));

	rc = circ_buf_pop(&cbuf, &j);
	PCUT_ASSERT_ERRNO_VAL(EAGAIN, rc);
}

PCUT_EXPORT(circ_buf);
