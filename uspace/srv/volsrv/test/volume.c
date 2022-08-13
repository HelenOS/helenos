/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
