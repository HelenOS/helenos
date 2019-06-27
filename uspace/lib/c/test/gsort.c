/*
 * Copyright (c) 2019 Matthieu Riolo
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
