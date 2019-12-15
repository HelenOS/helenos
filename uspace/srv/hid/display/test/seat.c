/*
 * Copyright (c) 2019 Jiri Svoboda
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
#include <stdio.h>
#include <str.h>

#include "../client.h"
#include "../display.h"
#include "../seat.h"
#include "../window.h"

PCUT_INIT;

PCUT_TEST_SUITE(seat);

static void test_ds_ev_pending(void *);

static ds_client_cb_t test_ds_client_cb = {
	.ev_pending = test_ds_ev_pending
};

static void test_ds_ev_pending(void *arg)
{
	bool *called_cb = (bool *) arg;
	printf("test_ds_ev_pending\n");
	*called_cb = true;

}

/** Set focus. */
PCUT_TEST(set_focus)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, wnd);
	PCUT_ASSERT_EQUALS(wnd, seat->focus);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Evacuate focus. */
PCUT_TEST(evac_focus)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, w1);
	PCUT_ASSERT_EQUALS(w1, seat->focus);

	ds_seat_evac_focus(seat, w1);
	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

PCUT_EXPORT(seat);
