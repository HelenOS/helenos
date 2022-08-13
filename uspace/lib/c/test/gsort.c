/*
 * SPDX-FileCopyrightText: 2019 Matthieu Riolo
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <gsort.h>

static int cmp_func(void *a, void *b, void *param)
{
	int ia = *(int *)a;
	int ib = *(int *)b;

	if (ia == ib)
		return 0;

	return ia < ib ? -1 : 1;
}

PCUT_INIT;

PCUT_TEST_SUITE(gsort);

/* sort ascending */
PCUT_TEST(gsort_asc)
{
	int size = 10;
	int data[size];

	for (int i = 0; i < size; i++) {
		data[i] = i;
	}

	bool ret = gsort(data, size, sizeof(int), cmp_func, NULL);
	PCUT_ASSERT_TRUE(ret);

	for (int i = 0; i < size; i++) {
		PCUT_ASSERT_INT_EQUALS(i, data[i]);
	}
}

/* sort ascending including double entries of the same number */
PCUT_TEST(gsort_asc_complex)
{
	int size = 10;
	int data[size];

	for (int i = 0; i < size; i++) {
		data[i] = (i * 13) % 9;
	}

	data[0] = 2;
	data[1] = 0;
	data[2] = 4;
	data[3] = 1;

	bool ret = gsort(data, size, sizeof(int), cmp_func, NULL);
	PCUT_ASSERT_TRUE(ret);

	int prev = data[0];
	for (int i = 1; i < size; i++) {
		PCUT_ASSERT_TRUE(prev <= data[i]);
		prev = data[i];
	}
}

/* sort descanding */
PCUT_TEST(gsort_desc)
{
	int size = 10;
	int data[size];

	for (int i = 0; i < size; i++) {
		data[i] = size - i;
	}

	bool ret = gsort(&data, size, sizeof(int), cmp_func, NULL);
	PCUT_ASSERT_TRUE(ret);

	for (int i = 0; i < size; i++) {
		PCUT_ASSERT_INT_EQUALS(i + 1, data[i]);
	}
}

PCUT_EXPORT(gsort);
