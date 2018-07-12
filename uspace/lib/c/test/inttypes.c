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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Test greatest-width integer types
 */

#include <pcut/pcut.h>
#include <inttypes.h>

PCUT_INIT;

PCUT_TEST_SUITE(inttypes);

PCUT_TEST(decls)
{
	/* Make sure intmax_t is defined */
	intmax_t i;
	(void) i;

	/* Make sure imaxdiv_t is defined */
	imaxdiv_t d;
	(void) d;
}

/** llabs function of positive number */
PCUT_TEST(imaxabs_pos)
{
	intmax_t i;

	i = imaxabs(1);
	PCUT_ASSERT_TRUE(i == 1);
}

/** llabs function of negative number */
PCUT_TEST(imaxabs_neg)
{
	intmax_t i;

	i = imaxabs(-1);
	PCUT_ASSERT_TRUE(i == 1);
}

/** Greatest-width integer division */
PCUT_TEST(imaxdiv)
{
	imaxdiv_t d;

	d = imaxdiv(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

PCUT_EXPORT(inttypes);
