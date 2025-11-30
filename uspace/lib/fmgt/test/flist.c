/*
 * Copyright (c) 2025 Jiri Svoboda
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

#include <fmgt.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(flist);

/** Create and destroy file list. */
PCUT_TEST(create_destroy)
{
	fmgt_flist_t *flist;
	errno_t rc;

	rc = fmgt_flist_create(&flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	fmgt_flist_destroy(flist);
}

/** Append entries to file list and walk them. */
PCUT_TEST(append_first_next)
{
	fmgt_flist_t *flist;
	fmgt_flist_entry_t *entry;
	errno_t rc;

	rc = fmgt_flist_create(&flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_flist_append(flist, "a");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_flist_append(flist, "b");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = fmgt_flist_first(flist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->fname);

	entry = fmgt_flist_next(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->fname);

	entry = fmgt_flist_next(entry);
	PCUT_ASSERT_NULL(entry);

	fmgt_flist_destroy(flist);
}

/** Append entries to file list and count them. */
PCUT_TEST(count)
{
	fmgt_flist_t *flist;
	errno_t rc;

	rc = fmgt_flist_create(&flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_flist_append(flist, "a");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = fmgt_flist_append(flist, "b");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, fmgt_flist_count(flist));

	fmgt_flist_destroy(flist);
}

PCUT_EXPORT(flist);
