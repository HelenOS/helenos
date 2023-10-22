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
#include <gfx/context.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <str.h>

#include "../client.h"
#include "../display.h"
#include "../idevcfg.h"
#include "../seat.h"
#include "../window.h"

PCUT_INIT;

PCUT_TEST_SUITE(window);

static errno_t dummy_set_color(void *, gfx_color_t *);
static errno_t dummy_fill_rect(void *, gfx_rect_t *);

static gfx_context_ops_t dummy_ops = {
	.set_color = dummy_set_color,
	.fill_rect = dummy_fill_rect
};

/** Test creating and destroying window */
PCUT_TEST(create_destroy)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_bring_to_top() brings window to top */
PCUT_TEST(bring_to_top)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w1;
	ds_window_t *w2;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* w2 should be on the top */
	PCUT_ASSERT_EQUALS(w2, ds_display_first_window(disp));

	/* Bring w1 to top */
	ds_window_bring_to_top(w1);

	/* Now w1 should be on the top */
	PCUT_ASSERT_EQUALS(w1, ds_display_first_window(disp));

	ds_window_destroy(w1);
	ds_window_destroy(w2);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_resize(). */
PCUT_TEST(window_resize)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_coord2_t offs;
	gfx_rect_t nrect;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd->dpos.x = 100;
	wnd->dpos.y = 100;

	offs.x = -2;
	offs.y = -3;
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = 12;
	params.rect.p1.y = 13;
	rc = ds_window_resize(wnd, &offs, &nrect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(98, wnd->dpos.x);
	PCUT_ASSERT_INT_EQUALS(97, wnd->dpos.y);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_get_pos(). */
PCUT_TEST(get_pos)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_coord2_t pos;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd->dpos.x = 100;
	wnd->dpos.y = 100;

	ds_window_get_pos(wnd, &pos);

	PCUT_ASSERT_INT_EQUALS(100, pos.x);
	PCUT_ASSERT_INT_EQUALS(100, pos.y);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_get_max_rect(). */
PCUT_TEST(get_max_rect)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_rect_t rect;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd->dpos.x = 100;
	wnd->dpos.y = 100;

	ds_window_get_max_rect(wnd, &rect);

	PCUT_ASSERT_INT_EQUALS(disp->rect.p0.x, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(disp->rect.p0.y, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(disp->rect.p1.x, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(disp->rect.p1.y, rect.p1.y);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_minimize(). */
PCUT_TEST(window_minimize)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_minimize(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(wndf_minimized, wnd->flags & wndf_minimized);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_unminimize(). */
PCUT_TEST(window_unminimize)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.flags |= wndf_minimized;
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_unminimize(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(0, wnd->flags & wndf_minimized);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_maximize(). */
PCUT_TEST(window_maximize)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd->dpos.x = 100;
	wnd->dpos.y = 100;

	rc = ds_window_maximize(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(wndf_maximized, wnd->flags & wndf_maximized);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_unmaximize(). */
PCUT_TEST(window_unmaximize)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd->dpos.x = 100;
	wnd->dpos.y = 100;

	rc = ds_window_maximize(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_unmaximize(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(0, wnd->flags & wndf_maximized);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_get_ctx(). */
PCUT_TEST(window_get_ctx)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_context_t *gc;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_window_get_ctx(wnd);
	PCUT_ASSERT_NOT_NULL(gc);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_is_visible() */
PCUT_TEST(is_visible)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 10;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(ds_window_is_visible(wnd));

	wnd->flags |= wndf_minimized;

	PCUT_ASSERT_FALSE(ds_window_is_visible(wnd));

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_post_kbd_event() with Alt-F4 sends close event */
PCUT_TEST(window_post_kbd_event_alt_f4)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	ds_window_t *rwindow;
	display_wnd_ev_t revent;
	display_wnd_params_t params;
	kbd_event_t event;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* New window gets focused event */
	rc = ds_client_get_event(client, &rwindow, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.mods = KM_ALT;
	event.key = KC_F4;
	rc = ds_window_post_kbd_event(wnd, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_get_event(client, &rwindow, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(wnd, rwindow);
	PCUT_ASSERT_EQUALS(wev_close, revent.etype);

	rc = ds_client_get_event(client, &rwindow, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_post_pos_event() */
PCUT_TEST(window_post_pos_event)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	pos_event_t event;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_idle, wnd->state);

	wnd->dpos.x = 10;
	wnd->dpos.y = 10;

	event.type = POS_PRESS;
	event.btn_num = 2;
	event.hpos = 10;
	event.vpos = 10;
	rc = ds_window_post_pos_event(wnd, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_moving, wnd->state);

	event.type = POS_UPDATE;
	event.hpos = 11;
	event.vpos = 12;
	rc = ds_window_post_pos_event(wnd, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_moving, wnd->state);
	/* Window position does not update until after release */
	PCUT_ASSERT_INT_EQUALS(10, wnd->dpos.x);
	PCUT_ASSERT_INT_EQUALS(10, wnd->dpos.y);

	event.type = POS_RELEASE;
	event.hpos = 13;
	event.vpos = 14;

	rc = ds_window_post_pos_event(wnd, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_idle, wnd->state);
	PCUT_ASSERT_INT_EQUALS(13, wnd->dpos.x);
	PCUT_ASSERT_INT_EQUALS(14, wnd->dpos.y);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_post_focus_event() */
PCUT_TEST(window_post_focus_event)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_post_focus_event(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_post_unfocus_event() */
PCUT_TEST(window_post_unfocus_event)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_post_focus_event(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_move_req() */
PCUT_TEST(window_move_req)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_idle, wnd->state);

	pos.x = 42;
	pos.y = 43;
	pos_id = 44;
	ds_window_move_req(wnd, &pos, pos_id);

	PCUT_ASSERT_INT_EQUALS(dsw_moving, wnd->state);
	PCUT_ASSERT_INT_EQUALS(pos.x, wnd->orig_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, wnd->orig_pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, wnd->orig_pos_id);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_resize_req() */
PCUT_TEST(window_resize_req)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_idle, wnd->state);

	pos.x = 42;
	pos.y = 43;
	pos_id = 44;
	ds_window_resize_req(wnd, display_wr_top_right, &pos, pos_id);

	PCUT_ASSERT_INT_EQUALS(dsw_resizing, wnd->state);
	PCUT_ASSERT_INT_EQUALS(display_wr_top_right, wnd->rsztype);
	PCUT_ASSERT_INT_EQUALS(pos.x, wnd->orig_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, wnd->orig_pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, wnd->orig_pos_id);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

PCUT_TEST(window_calc_resize)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_coord2_t dresize;
	gfx_coord2_t dresizen;
	gfx_coord2_t dresizeb;
	gfx_coord2_t dresizebn;
	gfx_rect_t nrect;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = 10;
	params.rect.p0.y = 11;
	params.rect.p1.x = 30;
	params.rect.p1.y = 31;
	params.min_size.x = 2;
	params.min_size.y = 3;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd->state = dsw_resizing;

	dresize.x = 5;
	dresize.y = 6;

	dresizen.x = -5;
	dresizen.y = -6;

	dresizeb.x = 50;
	dresizeb.y = 60;

	dresizebn.x = -50;
	dresizebn.y = -60;

	/* Resize top */
	wnd->rsztype = display_wr_top;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(17, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(5, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(28, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(-49, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	/* Resize top left */
	wnd->rsztype = display_wr_top_left;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(15, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(17, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(5, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(5, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(28, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(28, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(-40, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(-49, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	/* Resize left */
	wnd->rsztype = display_wr_left;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(15, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(5, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(28, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(-40, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	/* Resize bottom left */
	wnd->rsztype = display_wr_bottom_left;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(15, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(37, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(5, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(25, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(28, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(91, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(-40, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(14, nrect.p1.y);

	/* Resize bottom */
	wnd->rsztype = display_wr_bottom;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(37, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(25, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(91, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(30, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(14, nrect.p1.y);

	/* Resize bottom right */
	wnd->rsztype = display_wr_bottom_right;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(35, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(37, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(25, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(25, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(80, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(91, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(12, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(14, nrect.p1.y);

	/* Resize right */
	wnd->rsztype = display_wr_right;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(35, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(25, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(80, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(12, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	/* Resize top right */
	wnd->rsztype = display_wr_top_right;

	ds_window_calc_resize(wnd, &dresize, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(17, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(35, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizen, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(5, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(25, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizeb, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(28, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(80, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_calc_resize(wnd, &dresizebn, &nrect);
	PCUT_ASSERT_INT_EQUALS(10, nrect.p0.x);
	PCUT_ASSERT_INT_EQUALS(-49, nrect.p0.y);
	PCUT_ASSERT_INT_EQUALS(12, nrect.p1.x);
	PCUT_ASSERT_INT_EQUALS(31, nrect.p1.y);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_set_cursor() */
PCUT_TEST(window_set_cursor)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(wnd->display->cursor[dcurs_arrow], wnd->cursor);

	rc = ds_window_set_cursor(wnd, -1);
	PCUT_ASSERT_ERRNO_VAL(EINVAL, rc);
	PCUT_ASSERT_EQUALS(wnd->display->cursor[dcurs_arrow], wnd->cursor);

	// Check that invalid cursors cannot be set: ignore enum conversions
	// as we are out-of-bounds
#pragma GCC diagnostic push
#if defined(__GNUC__) && (__GNUC__ >= 10)
#pragma GCC diagnostic ignored "-Wenum-conversion"
#endif
	rc = ds_window_set_cursor(wnd, dcurs_limit);
	PCUT_ASSERT_ERRNO_VAL(EINVAL, rc);
	PCUT_ASSERT_EQUALS(wnd->display->cursor[dcurs_arrow], wnd->cursor);

	rc = ds_window_set_cursor(wnd, dcurs_limit + 1);
	PCUT_ASSERT_ERRNO_VAL(EINVAL, rc);
	PCUT_ASSERT_EQUALS(wnd->display->cursor[dcurs_arrow], wnd->cursor);
#pragma GCC diagnostic pop

	rc = ds_window_set_cursor(wnd, dcurs_size_lr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(wnd->display->cursor[dcurs_size_lr], wnd->cursor);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_set_caption() */
PCUT_TEST(window_set_caption)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(wnd->display->cursor[dcurs_arrow], wnd->cursor);

	rc = ds_window_set_caption(wnd, "Hello");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(0, str_cmp("Hello", wnd->caption));

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** ds_window_find_next() finds next window by flags */
PCUT_TEST(window_find_next)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	ds_window_t *w2;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	w1->flags |= wndf_minimized;

	rc = ds_window_create(client, &params, &w2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	w2->flags |= wndf_system;

	wnd = ds_window_find_next(w0, wndf_minimized);
	PCUT_ASSERT_EQUALS(w1, wnd);

	wnd = ds_window_find_next(w0, wndf_system);
	PCUT_ASSERT_EQUALS(w2, wnd);

	wnd = ds_window_find_next(w0, wndf_maximized);
	PCUT_ASSERT_NULL(wnd);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_window_destroy(w2);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** ds_window_find_prev() finds previous window by flags */
PCUT_TEST(window_find_prev)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	ds_window_t *w2;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	w2->flags |= wndf_system;

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	w1->flags |= wndf_minimized;

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd = ds_window_find_prev(w0, wndf_minimized);
	PCUT_ASSERT_EQUALS(w1, wnd);

	wnd = ds_window_find_prev(w0, wndf_system);
	PCUT_ASSERT_EQUALS(w2, wnd);

	wnd = ds_window_find_prev(w0, wndf_maximized);
	PCUT_ASSERT_NULL(wnd);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_window_destroy(w2);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** ds_window_unfocus() switches to another window */
PCUT_TEST(window_unfocus)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_unfocus(w0);

	PCUT_ASSERT_EQUALS(w1, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** ds_window_orig_seat() correctly compares seats */
PCUT_TEST(window_orig_seat)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat0;
	ds_seat_t *seat1;
	sysarg_t devid0;
	sysarg_t devid1;
	ds_idevcfg_t *cfg0;
	ds_idevcfg_t *cfg1;
	ds_window_t *wnd;
	display_wnd_params_t params;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Bob", &seat1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	devid0 = 42;
	devid1 = 43;

	rc = ds_idevcfg_create(disp, devid0, seat0, &cfg0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_idevcfg_create(disp, devid1, seat1, &cfg1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wnd->state = dsw_moving;
	wnd->orig_pos_id = devid0;

	PCUT_ASSERT_TRUE(ds_window_orig_seat(wnd, devid0));
	PCUT_ASSERT_FALSE(ds_window_orig_seat(wnd, devid1));

	ds_idevcfg_destroy(cfg0);
	ds_idevcfg_destroy(cfg1);
	ds_window_destroy(wnd);
	ds_seat_destroy(seat0);
	ds_seat_destroy(seat1);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

static errno_t dummy_set_color(void *arg, gfx_color_t *color)
{
	return EOK;
}

static errno_t dummy_fill_rect(void *arg, gfx_rect_t *rect)
{
	return EOK;
}

PCUT_EXPORT(window);
