/*
 * Copyright (c) 2013 Vojtech Horky
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

#include <stdio.h>
#include <pcut/pcut.h>
#include "tested.h"

PCUT_INIT;



PCUT_TEST_SUITE(with_teardown);

PCUT_TEST_AFTER
{
	printf("This is teardown-function.\n");
}

PCUT_TEST(empty)
{
}

PCUT_TEST(failing)
{
	PCUT_ASSERT_INT_EQUALS(10, intmin(1, 2));
}



PCUT_TEST_SUITE(with_failing_teardown);

PCUT_TEST_AFTER
{
	printf("This is failing teardown-function.\n");
	PCUT_ASSERT_INT_EQUALS(42, intmin(10, 20));
}

PCUT_TEST(empty2)
{
}

PCUT_TEST(printing2)
{
	printf("Printed before test failure.\n");
	PCUT_ASSERT_INT_EQUALS(0, intmin(-17, -19));
}

PCUT_TEST(failing2)
{
	PCUT_ASSERT_INT_EQUALS(12, intmin(3, 5));
}


PCUT_MAIN();
