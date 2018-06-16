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
 * @brief Test formatted input (scanf family)
 */

#include <pcut/pcut.h>
#include <stdlib.h>

PCUT_INIT;

PCUT_TEST_SUITE(stdlib);

PCUT_TEST(decls)
{
	/* Make sure size_t is defined */
	size_t sz = 0;
	(void) sz;

	/* Make sure wchar_t is defined */
	wchar_t wc = L'\0';
	(void) wc;

	/* Make sure EXIT_FAILURE and EXIT_SUCCESS are defined */
	if (0)
		exit(EXIT_FAILURE);
	if (0)
		exit(EXIT_SUCCESS);

	/* Make sure NULL is defined */
	void *ptr = NULL;
	(void) ptr;
}

/** Integer division */
PCUT_TEST(div_func)
{
	div_t d;

	d = div(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

/** Long integer division */
PCUT_TEST(ldiv_func)
{
	ldiv_t d;

	d = ldiv(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

/** Long long integer division */
PCUT_TEST(lldiv_func)
{
	lldiv_t d;

	d = lldiv(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

PCUT_EXPORT(stdlib);
