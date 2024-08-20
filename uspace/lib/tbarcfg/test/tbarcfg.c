/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <stdbool.h>
#include <stdio.h>

PCUT_INIT;

PCUT_TEST_SUITE(tbarcfg);

typedef struct {
	bool notified;
} tbarcfg_test_resp_t;

static void test_cb(void *);

/** Creating, opening and closing taskbar configuration */
PCUT_TEST(create_open_close)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Create new repository */
	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tbarcfg_sync(tbcfg);
	tbarcfg_close(tbcfg);

	/* Re-open the repository */

	rc = tbarcfg_open(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Iterating over start menu entries */
PCUT_TEST(first_next)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e1 = NULL, *e2 = NULL;
	smenu_entry_t *e;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e1);

	rc = smenu_entry_create(tbcfg, "B", "b", false, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e2);

	/* Create entry without getting a pointer to it */
	rc = smenu_entry_create(tbcfg, "C", "c", false, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_EQUALS(e1, e);
	e = tbarcfg_smenu_next(e);
	PCUT_ASSERT_EQUALS(e2, e);
	e = tbarcfg_smenu_next(e);
	PCUT_ASSERT_NOT_NULL(e);
	e = tbarcfg_smenu_next(e);
	PCUT_ASSERT_NULL(e);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Iterating over start menu entries backwards */
PCUT_TEST(last_prev)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e1 = NULL, *e2 = NULL;
	smenu_entry_t *e;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e1);

	rc = smenu_entry_create(tbcfg, "B", "b", false, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e2);

	e = tbarcfg_smenu_last(tbcfg);
	PCUT_ASSERT_EQUALS(e2, e);
	e = tbarcfg_smenu_prev(e);
	PCUT_ASSERT_EQUALS(e1, e);
	e = tbarcfg_smenu_prev(e);
	PCUT_ASSERT_NULL(e);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Separator entry */
PCUT_TEST(separator)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	const char *caption;
	const char *cmd;
	smenu_entry_t *e1 = NULL, *e2 = NULL;
	smenu_entry_t *e;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e1);

	rc = smenu_entry_sep_create(tbcfg, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e2);

	PCUT_ASSERT_FALSE(smenu_entry_get_separator(e1));
	PCUT_ASSERT_TRUE(smenu_entry_get_separator(e2));

	tbarcfg_sync(tbcfg);
	tbarcfg_close(tbcfg);

	/* Re-open repository */

	rc = tbarcfg_open(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_NOT_NULL(e);

	/* Check that new values of properties have persisted */
	PCUT_ASSERT_FALSE(smenu_entry_get_separator(e));
	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("A", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("a", cmd);

	e = tbarcfg_smenu_next(e);

	/* Check that entry is still a separator */
	PCUT_ASSERT_TRUE(smenu_entry_get_separator(e));

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Getting menu entry properties */
PCUT_TEST(get_caption_cmd_term)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e;
	const char *caption;
	const char *cmd;
	bool terminal;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("A", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("a", cmd);
	terminal = smenu_entry_get_terminal(e);
	PCUT_ASSERT_FALSE(terminal);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Setting menu entry properties */
PCUT_TEST(set_caption_cmd_term)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e;
	const char *caption;
	const char *cmd;
	bool terminal;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("A", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("a", cmd);
	terminal = smenu_entry_get_terminal(e);
	PCUT_ASSERT_FALSE(terminal);

	/* Set properties */
	rc = smenu_entry_set_caption(e, "B");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = smenu_entry_set_cmd(e, "b");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	smenu_entry_set_terminal(e, true);

	rc = tbarcfg_sync(tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Check that properties have been set */
	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("B", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("b", cmd);
	terminal = smenu_entry_get_terminal(e);
	PCUT_ASSERT_TRUE(terminal);

	tbarcfg_sync(tbcfg);
	tbarcfg_close(tbcfg);

	/* Re-open repository */

	rc = tbarcfg_open(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_NOT_NULL(e);

	/* Check that new values of properties have persisted */
	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("B", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("b", cmd);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Create start menu entry */
PCUT_TEST(entry_create)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e;
	const char *caption;
	const char *cmd;
	bool terminal;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e);

	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("A", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("a", cmd);
	terminal = smenu_entry_get_terminal(e);
	PCUT_ASSERT_FALSE(terminal);

	smenu_entry_destroy(e);

	rc = smenu_entry_create(tbcfg, "B", "b", true, &e);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e);

	caption = smenu_entry_get_caption(e);
	PCUT_ASSERT_STR_EQUALS("B", caption);
	cmd = smenu_entry_get_cmd(e);
	PCUT_ASSERT_STR_EQUALS("b", cmd);
	terminal = smenu_entry_get_terminal(e);
	PCUT_ASSERT_TRUE(terminal);

	smenu_entry_destroy(e);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Destroy start menu entry */
PCUT_TEST(entry_destroy)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e, *f;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_EQUALS(e, f);

	smenu_entry_destroy(e);

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_NULL(f);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Move start menu entry up */
PCUT_TEST(entry_move_up)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e1, *e2, *e3;
	smenu_entry_t *f;
	const char *caption;
	const char *cmd;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "B", "b", false, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "C", "c", false, &e3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_EQUALS(e1, f);

	/* Moving the first entry up should have no effect */

	smenu_entry_move_up(e1);

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_EQUALS(e1, f);

	/* Moving the second entry up should move it to first position */

	smenu_entry_move_up(e2);

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_EQUALS(e2, f);

	/* Moving the last entry up should move it to second position */

	smenu_entry_move_up(e3);

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_EQUALS(e2, f);

	f = tbarcfg_smenu_next(f);
	PCUT_ASSERT_EQUALS(e3, f);

	f = tbarcfg_smenu_next(f);
	PCUT_ASSERT_EQUALS(e1, f);

	tbarcfg_sync(tbcfg);
	tbarcfg_close(tbcfg);

	/* Re-open repository */

	rc = tbarcfg_open(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Check that new order of entries persisted */

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_NOT_NULL(f);

	caption = smenu_entry_get_caption(f);
	PCUT_ASSERT_STR_EQUALS("B", caption);
	cmd = smenu_entry_get_cmd(f);
	PCUT_ASSERT_STR_EQUALS("b", cmd);

	f = tbarcfg_smenu_next(f);
	PCUT_ASSERT_NOT_NULL(f);

	caption = smenu_entry_get_caption(f);
	PCUT_ASSERT_STR_EQUALS("C", caption);
	cmd = smenu_entry_get_cmd(f);
	PCUT_ASSERT_STR_EQUALS("c", cmd);

	f = tbarcfg_smenu_next(f);
	PCUT_ASSERT_NOT_NULL(f);

	caption = smenu_entry_get_caption(f);
	PCUT_ASSERT_STR_EQUALS("A", caption);
	cmd = smenu_entry_get_cmd(f);
	PCUT_ASSERT_STR_EQUALS("a", cmd);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Move start menu entry down */
PCUT_TEST(entry_move_down)
{
	errno_t rc;
	tbarcfg_t *tbcfg;
	char fname[L_tmpnam], *p;
	smenu_entry_t *e1, *e2, *e3;
	smenu_entry_t *f;
	const char *caption;
	const char *cmd;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = tbarcfg_create(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "A", "a", false, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "B", "b", false, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = smenu_entry_create(tbcfg, "C", "c", false, &e3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	f = tbarcfg_smenu_last(tbcfg);
	PCUT_ASSERT_EQUALS(e3, f);

	/* Moving the last entry down should have no effect */

	smenu_entry_move_down(e3);

	f = tbarcfg_smenu_last(tbcfg);
	PCUT_ASSERT_EQUALS(e3, f);

	/* Moving the second entry down should move it to last position */

	smenu_entry_move_down(e2);

	f = tbarcfg_smenu_last(tbcfg);
	PCUT_ASSERT_EQUALS(e2, f);

	/* Moving the first entry down should move it to second position */

	smenu_entry_move_down(e1);

	f = tbarcfg_smenu_last(tbcfg);
	PCUT_ASSERT_EQUALS(e2, f);

	f = tbarcfg_smenu_prev(f);
	PCUT_ASSERT_EQUALS(e1, f);

	f = tbarcfg_smenu_prev(f);
	PCUT_ASSERT_EQUALS(e3, f);

	tbarcfg_sync(tbcfg);
	tbarcfg_close(tbcfg);

	/* Re-open repository */

	rc = tbarcfg_open(fname, &tbcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Check that new order of entries persisted */

	f = tbarcfg_smenu_first(tbcfg);
	PCUT_ASSERT_NOT_NULL(f);

	caption = smenu_entry_get_caption(f);
	PCUT_ASSERT_STR_EQUALS("C", caption);
	cmd = smenu_entry_get_cmd(f);
	PCUT_ASSERT_STR_EQUALS("c", cmd);

	f = tbarcfg_smenu_next(f);
	PCUT_ASSERT_NOT_NULL(f);

	caption = smenu_entry_get_caption(f);
	PCUT_ASSERT_STR_EQUALS("A", caption);
	cmd = smenu_entry_get_cmd(f);
	PCUT_ASSERT_STR_EQUALS("a", cmd);

	f = tbarcfg_smenu_next(f);
	PCUT_ASSERT_NOT_NULL(f);

	caption = smenu_entry_get_caption(f);
	PCUT_ASSERT_STR_EQUALS("B", caption);
	cmd = smenu_entry_get_cmd(f);
	PCUT_ASSERT_STR_EQUALS("b", cmd);

	tbarcfg_close(tbcfg);
	remove(fname);
}

/** Notifications can be delivered from tbarcfg_notify() to a listener. */
PCUT_TEST(notify)
{
	errno_t rc;
	tbarcfg_listener_t *lst;
	tbarcfg_test_resp_t test_resp;

	test_resp.notified = false;

	printf("create listener resp=%p\n", (void *)&test_resp);
	rc = tbarcfg_listener_create(TBARCFG_NOTIFY_DEFAULT,
	    test_cb, &test_resp, &lst);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = tbarcfg_notify(TBARCFG_NOTIFY_DEFAULT);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(test_resp.notified);
	tbarcfg_listener_destroy(lst);
}

static void test_cb(void *arg)
{
	tbarcfg_test_resp_t *resp = (tbarcfg_test_resp_t *)arg;

	printf("test_cb: executing resp=%p\n", (void *)resp);
	resp->notified = true;
}

PCUT_EXPORT(tbarcfg);
