/*
 * Copyright (c) 2015 Michal Koutny
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

#include <adt/dyn_array.h>
#include <assert.h>
#include <pcut/pcut.h>

PCUT_INIT

PCUT_TEST_SUITE(dyn_array);

typedef int data_t;
static dyn_array_t da;

PCUT_TEST_BEFORE {
	dyn_array_initialize(&da, data_t);
	int rc = dyn_array_reserve(&da, 3);
	assert(rc == EOK);
}

PCUT_TEST_AFTER {
	dyn_array_destroy(&da);
}

PCUT_TEST(initialization) {
	PCUT_ASSERT_INT_EQUALS(da.capacity, 3);
	PCUT_ASSERT_INT_EQUALS(da.size, 0);
}

PCUT_TEST(append) {
	dyn_array_append(&da, data_t, 42);
	dyn_array_append(&da, data_t, 666);

	PCUT_ASSERT_INT_EQUALS(2, da.size);
	PCUT_ASSERT_INT_EQUALS(42, dyn_array_at(&da, data_t, 0));
	PCUT_ASSERT_INT_EQUALS(666, dyn_array_at(&da, data_t, 1));
}

PCUT_TEST(assign) {
	dyn_array_append(&da, data_t, 42);
	dyn_array_at(&da, data_t, 0) = 112;

	PCUT_ASSERT_INT_EQUALS(112, dyn_array_at(&da, data_t, 0));
}

PCUT_TEST(remove) {
	dyn_array_append(&da, data_t, 10);
	dyn_array_append(&da, data_t, 11);

	dyn_array_remove(&da, 0);

	PCUT_ASSERT_INT_EQUALS(1, da.size);
	PCUT_ASSERT_INT_EQUALS(11, dyn_array_at(&da, data_t, 0));
}

PCUT_TEST(insert) {
	dyn_array_append(&da, data_t, 10);
	dyn_array_append(&da, data_t, 11);
	dyn_array_append(&da, data_t, 12);
	dyn_array_insert(&da, data_t, 1, 99);

	PCUT_ASSERT_INT_EQUALS(4, da.size);
	PCUT_ASSERT_INT_EQUALS(10, dyn_array_at(&da, data_t, 0));
	PCUT_ASSERT_INT_EQUALS(99, dyn_array_at(&da, data_t, 1));
	PCUT_ASSERT_INT_EQUALS(11, dyn_array_at(&da, data_t, 2));
	PCUT_ASSERT_INT_EQUALS(12, dyn_array_at(&da, data_t, 3));
}

PCUT_TEST(capacity_grow) {
	dyn_array_append(&da, data_t, 42);
	dyn_array_append(&da, data_t, 666);
	dyn_array_append(&da, data_t, 42);
	dyn_array_append(&da, data_t, 666);

	PCUT_ASSERT_TRUE(da.capacity > 3);
}

PCUT_TEST(capacity_shrink) {
	dyn_array_append(&da, data_t, 42);
	dyn_array_append(&da, data_t, 666);
	dyn_array_append(&da, data_t, 42);

	dyn_array_remove(&da, 0);
	dyn_array_remove(&da, 0);
	dyn_array_remove(&da, 0);

	PCUT_ASSERT_TRUE(da.capacity < 3);
}

PCUT_TEST(iterator) {
	for (int i = 0; i < 10; ++i) {
		dyn_array_append(&da, data_t, i*i);
	}

	int i = 0;
	dyn_array_foreach(da, data_t, it) {
		PCUT_ASSERT_INT_EQUALS(i*i, *it);
		++i;
	}
}

PCUT_TEST(find) {
	dyn_array_append(&da, data_t, 10);
	dyn_array_append(&da, data_t, 11);
	dyn_array_append(&da, data_t, 12);
	dyn_array_append(&da, data_t, 99);

	PCUT_ASSERT_INT_EQUALS(0, dyn_array_find(&da, data_t, 10));
	PCUT_ASSERT_INT_EQUALS(3, dyn_array_find(&da, data_t, 99));
	PCUT_ASSERT_INT_EQUALS(4, dyn_array_find(&da, data_t, 666));
}

PCUT_EXPORT(dyn_array);
