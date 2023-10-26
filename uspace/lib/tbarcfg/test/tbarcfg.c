/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <tbarcfg/tbarcfg.h>
#include <stdio.h>

PCUT_INIT;

PCUT_TEST_SUITE(tbarcfg);

/** Opening and closing taskbar configuration */
PCUT_TEST(open_close)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	FILE *f;
	int rv;

	f = fopen("/tmp/test", "wt");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fputs("[sif](){[entries](){}}", f);
	PCUT_ASSERT_TRUE(rv >= 0);

	rv = fclose(f);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rc = tbarcfg_open("/tmp/test", &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tbarcfg_close(tbcfg);
}

/** Iterating over start menu entries */
PCUT_TEST(first_next)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	smenu_entry_t *e;
	FILE *f;
	int rv;

	f = fopen("/tmp/test", "wt");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fputs("[sif](){[entries](){"
	    "[entry]([caption]=[A][cmd]=[a]){}"
	    "[entry]([caption]=[B][cmd]=[b]){}"
	    "}}", f);
	PCUT_ASSERT_TRUE(rv >= 0);

	rv = fclose(f);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rc = tbarcfg_open("/tmp/test", &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_NOT_NULL(e);
	e = tbarcfg_smenu_next(e);
	PCUT_ASSERT_NOT_NULL(e);
	e = tbarcfg_smenu_next(e);
	PCUT_ASSERT_NULL(e);

	tbarcfg_close(tbcfg);
}

/** Getting menu entry properties */
PCUT_TEST(get_caption_cmd)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	smenu_entry_t *e;
	const char *caption;
	const char *cmd;
	FILE *f;
	int rv;

	f = fopen("/tmp/test", "wt");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fputs("[sif](){[entries](){"
	    "[entry]([caption]=[A][cmd]=[a]){}"
	    "}}", f);
	PCUT_ASSERT_TRUE(rv >= 0);

	rv = fclose(f);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rc = tbarcfg_open("/tmp/test", &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_NOT_NULL(e);

	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("A", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("a", cmd);

	tbarcfg_close(tbcfg);
}

PCUT_EXPORT(tbarcfg);
