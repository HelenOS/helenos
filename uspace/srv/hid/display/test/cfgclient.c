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

#include <disp_srv.h>
#include <errno.h>
#include <pcut/pcut.h>
#include <str.h>

#include "../display.h"
#include "../cfgclient.h"

PCUT_INIT;

PCUT_TEST_SUITE(cfgclient);

static void test_ds_dcev_pending(void *);

static ds_cfgclient_cb_t test_ds_cfgclient_cb = {
	.ev_pending = test_ds_dcev_pending
};

static void test_ds_dcev_pending(void *arg)
{
	bool *called_cb = (bool *) arg;
	*called_cb = true;
}

/** WM client creation and destruction. */
PCUT_TEST(create_destroy)
{
	ds_display_t *disp;
	ds_cfgclient_t *cfgclient;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cfgclient_create(disp, &test_ds_cfgclient_cb, NULL, &cfgclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_cfgclient_destroy(cfgclient);
	ds_display_destroy(disp);
}

/** Test ds_cfgclient_get_event(), ds_cfgclient_post_seat_added_event(). */
PCUT_TEST(client_get_post_seat_added_event)
{
	ds_display_t *disp;
	ds_cfgclient_t *cfgclient;
	dispcfg_ev_t revent;
	bool called_cb = NULL;
	sysarg_t seat_id;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cfgclient_create(disp, &test_ds_cfgclient_cb, &called_cb,
	    &cfgclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	called_cb = false;
	seat_id = 42;

	rc = ds_cfgclient_get_event(cfgclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = ds_cfgclient_post_seat_added_event(cfgclient, seat_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);

	rc = ds_cfgclient_get_event(cfgclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(seat_id, revent.seat_id);
	PCUT_ASSERT_EQUALS(dcev_seat_added, revent.etype);

	rc = ds_cfgclient_get_event(cfgclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_cfgclient_destroy(cfgclient);
	ds_display_destroy(disp);
}

/** Test ds_cfgclient_get_event(), ds_cfgclient_post_seat_removed_event(). */
PCUT_TEST(client_get_post_seat_removed_event)
{
	ds_display_t *disp;
	ds_cfgclient_t *cfgclient;
	dispcfg_ev_t revent;
	bool called_cb = NULL;
	sysarg_t seat_id;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cfgclient_create(disp, &test_ds_cfgclient_cb, &called_cb,
	    &cfgclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	called_cb = false;
	seat_id = 42;

	rc = ds_cfgclient_get_event(cfgclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = ds_cfgclient_post_seat_removed_event(cfgclient, seat_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);

	rc = ds_cfgclient_get_event(cfgclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(seat_id, revent.seat_id);
	PCUT_ASSERT_EQUALS(dcev_seat_removed, revent.etype);

	rc = ds_cfgclient_get_event(cfgclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_cfgclient_destroy(cfgclient);
	ds_display_destroy(disp);
}

/** Test ds_cfgclient_purge_events() */
PCUT_TEST(purge_events)
{
	ds_display_t *disp;
	ds_cfgclient_t *cfgclient;
	dispcfg_ev_t revent;
	bool called_cb = NULL;
	sysarg_t seat_id;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cfgclient_create(disp, &test_ds_cfgclient_cb, &called_cb,
	    &cfgclient);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Post window added event */
	seat_id = 42;
	rc = ds_cfgclient_post_seat_added_event(cfgclient, seat_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Purge it */
	ds_cfgclient_purge_events(cfgclient);

	/* The queue should be empty now */
	rc = ds_cfgclient_get_event(cfgclient, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_cfgclient_destroy(cfgclient);
	ds_display_destroy(disp);
}

PCUT_EXPORT(cfgclient);
