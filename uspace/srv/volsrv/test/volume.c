/*
 * Copyright (c) 2018 Jiri Svoboda
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

#include "../volume.h"

PCUT_INIT;

PCUT_TEST_SUITE(volume);

/** Volumes list creation and destruction. */
PCUT_TEST(volumes_basic)
{
	vol_volumes_t *volumes;
	char *namebuf;
	char *fname;
	errno_t rc;
	int rv;

	namebuf = malloc(L_tmpnam);
	PCUT_ASSERT_NOT_NULL(namebuf);

	fname = tmpnam(namebuf);
	PCUT_ASSERT_NOT_NULL(fname);

	rc = vol_volumes_create(fname, &volumes);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	vol_volumes_destroy(volumes);
	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
	free(fname);
}

/** Two references to the same volume, reference to a different volume. */
PCUT_TEST(two_same_different)
{
	vol_volumes_t *volumes;
	vol_volume_t *va, *vb, *va1;
	char *namebuf;
	char *fname;
	errno_t rc;
	int rv;

	namebuf = malloc(L_tmpnam);
	PCUT_ASSERT_NOT_NULL(namebuf);

	fname = tmpnam(namebuf);
	PCUT_ASSERT_NOT_NULL(fname);

	rc = vol_volumes_create(fname, &volumes);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = vol_volume_lookup_ref(volumes, "foo", &va);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = vol_volume_lookup_ref(volumes, "bar", &vb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = vol_volume_lookup_ref(volumes, "foo", &va1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(va1 == va);

	vol_volume_del_ref(va);
	vol_volume_del_ref(vb);
	vol_volume_del_ref(va1);

	vol_volumes_destroy(volumes);
	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
	free(fname);
}

/** Reference the same volume twice, making sure it persists. */
PCUT_TEST(same_twice)
{
	vol_volumes_t *volumes;
	vol_volume_t *va;
	char *namebuf;
	char *fname;
	errno_t rc;
	int rv;

	namebuf = malloc(L_tmpnam);
	PCUT_ASSERT_NOT_NULL(namebuf);

	fname = tmpnam(namebuf);
	PCUT_ASSERT_NOT_NULL(fname);

	rc = vol_volumes_create(fname, &volumes);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Look up a volume */
	rc = vol_volume_lookup_ref(volumes, "foo", &va);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Setting mount point forces it to persist after dropping the ref. */
	rc = vol_volume_set_mountp(va, "/xyz");

	/* Drop the reference */
	vol_volume_del_ref(va);

	/* Look up volume again */
	rc = vol_volume_lookup_ref(volumes, "foo", &va);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* The mount point should still be set */
	PCUT_ASSERT_NOT_NULL(va->mountp);
	PCUT_ASSERT_TRUE(str_cmp(va->mountp, "/xyz") == 0);

	vol_volume_del_ref(va);

	vol_volumes_destroy(volumes);
	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);
	free(fname);
}

PCUT_EXPORT(volume);
