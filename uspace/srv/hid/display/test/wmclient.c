/*
 * Copyright (c) 2022 Jiri Svoboda
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

#include <disp_srv.h>
#include <errno.h>
#include <pcut/pcut.h>
#include <str.h>

#include "../display.h"
#include "../wmclient.h"

PCUT_INIT;

PCUT_TEST_SUITE(wmclient);

static void test_ds_wmev_pending(void *);

static ds_wmclient_cb_t test_ds_wmclient_cb = {
	.ev_pending = test_ds_wmev_pending
};

static void test_ds_wmev_pending(void *arg)
{
	bool *called_cb = (bool *) arg;
	*called_cb = true;
}

/** WM client creation and destruction. */
PCUT_TEST(create_destroy)
{
	ds_display_t *disp;
	ds_wmclient_t *wmclient;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_wmclient_create(disp, &test_ds_wmclient_cb, NULL, &wmclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_wmclient_destroy(wmclient);
	ds_display_destroy(disp);
}

/** Test ds_wmclient_get_event(), ds_wmclient_post_wnd_added_event(). */
PCUT_TEST(client_get_post_wnd_added_event)
{
	ds_display_t *disp;
	ds_wmclient_t *wmclient;
	wndmgt_ev_t revent;
	bool called_cb = NULL;
	sysarg_t wnd_id;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_wmclient_create(disp, &test_ds_wmclient_cb, &called_cb,
	    &wmclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	called_cb = false;
	wnd_id = 42;

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = ds_wmclient_post_wnd_added_event(wmclient, wnd_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(wnd_id, revent.wnd_id);
	PCUT_ASSERT_EQUALS(wmev_window_added, revent.etype);

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_wmclient_destroy(wmclient);
	ds_display_destroy(disp);
}

/** Test ds_wmclient_get_event(), ds_wmclient_post_wnd_removed_event(). */
PCUT_TEST(client_get_post_wnd_removed_event)
{
	ds_display_t *disp;
	ds_wmclient_t *wmclient;
	wndmgt_ev_t revent;
	bool called_cb = NULL;
	sysarg_t wnd_id;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_wmclient_create(disp, &test_ds_wmclient_cb, &called_cb,
	    &wmclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	called_cb = false;
	wnd_id = 42;

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = ds_wmclient_post_wnd_removed_event(wmclient, wnd_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(wnd_id, revent.wnd_id);
	PCUT_ASSERT_EQUALS(wmev_window_removed, revent.etype);

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_wmclient_destroy(wmclient);
	ds_display_destroy(disp);
}

/** Test ds_wmclient_get_event(), ds_wmclient_post_wnd_changed_event(). */
PCUT_TEST(client_get_post_wnd_changed_event)
{
	ds_display_t *disp;
	ds_wmclient_t *wmclient;
	wndmgt_ev_t revent;
	bool called_cb = NULL;
	sysarg_t wnd_id;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_wmclient_create(disp, &test_ds_wmclient_cb, &called_cb,
	    &wmclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	called_cb = false;
	wnd_id = 42;

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = ds_wmclient_post_wnd_changed_event(wmclient, wnd_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(wnd_id, revent.wnd_id);
	PCUT_ASSERT_EQUALS(wmev_window_changed, revent.etype);

	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_wmclient_destroy(wmclient);
	ds_display_destroy(disp);
}

/** Test ds_wmclient_purge_events() */
PCUT_TEST(purge_events)
{
	ds_display_t *disp;
	ds_wmclient_t *wmclient;
	wndmgt_ev_t revent;
	bool called_cb = NULL;
	sysarg_t wnd_id;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_wmclient_create(disp, &test_ds_wmclient_cb, &called_cb,
	    &wmclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Post window added event */
	wnd_id = 42;
	rc = ds_wmclient_post_wnd_added_event(wmclient, wnd_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Purge it */
	ds_wmclient_purge_events(wmclient);

	/* The queue should be empty now */
	rc = ds_wmclient_get_event(wmclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_wmclient_destroy(wmclient);
	ds_display_destroy(disp);
}

PCUT_EXPORT(wmclient);
