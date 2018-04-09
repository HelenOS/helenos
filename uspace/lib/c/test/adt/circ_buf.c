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
