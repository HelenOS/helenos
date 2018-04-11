/*
 * Copyright (c) 2014 Vojtech Horky
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
#include "tested.h"

PCUT_INIT;

PCUT_TEST(int_equals)
{
	PCUT_ASSERT_INT_EQUALS(1, 1);
	PCUT_ASSERT_INT_EQUALS(1, 0);
}

PCUT_TEST(double_equals)
{
	PCUT_ASSERT_DOUBLE_EQUALS(1., 1., 0.001);
	PCUT_ASSERT_DOUBLE_EQUALS(1., 0.5, 0.4);
}

PCUT_TEST(str_equals)
{
	PCUT_ASSERT_STR_EQUALS("xyz", "xyz");
	PCUT_ASSERT_STR_EQUALS("abc", "xyz");
}

PCUT_TEST(str_equals_or_null_base)
{
	PCUT_ASSERT_STR_EQUALS_OR_NULL("xyz", "xyz");
}

PCUT_TEST(str_equals_or_null_different)
{
	PCUT_ASSERT_STR_EQUALS_OR_NULL("abc", "xyz");
}

PCUT_TEST(str_equals_or_null_one_null)
{
	PCUT_ASSERT_STR_EQUALS_OR_NULL(NULL, "xyz");
}

PCUT_TEST(str_equals_or_null_both)
{
	PCUT_ASSERT_STR_EQUALS_OR_NULL(NULL, NULL);
}

PCUT_TEST(assert_true)
{
	PCUT_ASSERT_TRUE(42);
	PCUT_ASSERT_TRUE(0);
}

PCUT_TEST(assert_false)
{
	PCUT_ASSERT_FALSE(0);
	PCUT_ASSERT_FALSE(42);
}

PCUT_MAIN();
