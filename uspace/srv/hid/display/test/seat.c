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
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, wnd);
	PCUT_ASSERT_EQUALS(wnd, seat->focus);
	PCUT_ASSERT_TRUE(called_cb);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Evacuate focus from window.
 *
 * After evacuating no window should be focused
 */
PCUT_TEST(evac_focus_one_window)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, wnd);
	PCUT_ASSERT_EQUALS(wnd, seat->focus);
	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	ds_seat_evac_wnd_refs(seat, wnd);
	PCUT_ASSERT_NULL(seat->focus);
	PCUT_ASSERT_TRUE(called_cb);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Evacuate popup reference from window.
 *
 * After evacuating no window should be set as the popup
 */
PCUT_TEST(evac_popup)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;
	params.flags |= wndf_popup;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(wnd, seat->popup);

	ds_seat_evac_wnd_refs(seat, wnd);
	PCUT_ASSERT_NULL(seat->popup);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Unfocus window when three windows are available */
PCUT_TEST(unfocus_wnd_three_windows)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	ds_window_t *w2;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* w0 is at the top, then w1, then w2 */

	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_unfocus(w0);

	/* The previous window, w1, should be focused now */
	PCUT_ASSERT_EQUALS(w1, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Unfocus window when two windows and one system window are available */
PCUT_TEST(unfocus_wnd_two_windows_one_sys)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	ds_window_t *w2;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	params.flags |= wndf_system;
	rc = ds_window_create(client, &params, &w2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	params.flags &= ~wndf_system;
	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_unfocus(w0);

	/* The previous non-system window is w1, w2 should be skipped */
	PCUT_ASSERT_EQUALS(w1, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Unfocus window when one window and one system window is available */
PCUT_TEST(unfocus_wnd_one_window_one_sys)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	params.flags |= wndf_system;
	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	params.flags &= ~wndf_system;
	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_unfocus(w0);

	/* Since no non-system window is available, w1 should be focused */
	PCUT_ASSERT_EQUALS(w1, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Unfocus window when one window is available */
PCUT_TEST(unfocus_wnd_one_window)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_unfocus(w0);

	/* Since no other window is availabe, no window should be focused */
	PCUT_ASSERT_EQUALS(NULL, seat->focus);

	ds_window_destroy(w0);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Switch focus when another window is available. */
PCUT_TEST(switch_focus_two_windows)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
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

	ds_seat_set_focus(seat, w1);
	PCUT_ASSERT_EQUALS(w1, seat->focus);
	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	ds_seat_switch_focus(seat);
	PCUT_ASSERT_EQUALS(w0, seat->focus);
	PCUT_ASSERT_TRUE(called_cb);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Switch focus with just one existing window.
 *
 * After switching the focus should remain with the same window.
 */
PCUT_TEST(switch_focus_one_window)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, wnd);
	PCUT_ASSERT_EQUALS(wnd, seat->focus);
	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	ds_seat_switch_focus(seat);
	PCUT_ASSERT_EQUALS(wnd, seat->focus);
	PCUT_ASSERT_FALSE(called_cb);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_seat_post_kbd_event() with Alt-Tab switches focus */
PCUT_TEST(post_kbd_event_alt_tab)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	display_wnd_params_t params;
	bool called_cb = false;
	kbd_event_t event;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
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

	ds_seat_set_focus(seat, w1);
	PCUT_ASSERT_EQUALS(w1, seat->focus);
	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	event.type = KEY_PRESS;
	event.mods = KM_ALT;
	event.key = KC_TAB;
	rc = ds_seat_post_kbd_event(seat, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(w0, seat->focus);
	PCUT_ASSERT_TRUE(called_cb);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_seat_post_kbd_event() with regular key press delivers to client queue */
PCUT_TEST(post_kbd_event_regular)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *wnd;
	display_wnd_params_t params;
	kbd_event_t event;
	ds_window_t *rwindow;
	display_wnd_ev_t revent;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, wnd);
	PCUT_ASSERT_EQUALS(wnd, seat->focus);

	PCUT_ASSERT_TRUE(called_cb);
	rc = ds_client_get_event(client, &rwindow, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	called_cb = false;

	event.type = KEY_PRESS;
	event.key = KC_ENTER;
	event.mods = 0;
	event.c = L'\0';

	rc = ds_client_get_event(client, &rwindow, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = ds_seat_post_kbd_event(seat, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);

	rc = ds_client_get_event(client, &rwindow, &revent);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(wnd, rwindow);
	PCUT_ASSERT_EQUALS(wev_kbd, revent.etype);
	PCUT_ASSERT_EQUALS(event.type, revent.ev.kbd.type);
	PCUT_ASSERT_EQUALS(event.key, revent.ev.kbd.key);
	PCUT_ASSERT_EQUALS(event.mods, revent.ev.kbd.mods);
	PCUT_ASSERT_EQUALS(event.c, revent.ev.kbd.c);

	rc = ds_client_get_event(client, &rwindow, &revent);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	ds_window_destroy(wnd);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_seat_post_ptd_event() with click on window switches focus
 */
PCUT_TEST(post_ptd_event_wnd_switch)
{
	ds_display_t *disp;
	ds_seat_t *seat;
	ds_client_t *client;
	ds_window_t *w0, *w1;
	display_wnd_params_t params;
	ptd_event_t event;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Set up display size to allow the pointer a range of movement */
	disp->rect.p1.x = 500;
	disp->rect.p1.y = 500;

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	w0->dpos.x = 10;
	w0->dpos.y = 10;

	w1->dpos.x = 400;
	w1->dpos.y = 400;

	/* New window gets focused event */
	PCUT_ASSERT_TRUE(called_cb);

	called_cb = false;

	ds_seat_set_focus(seat, w0);

	event.type = PTD_MOVE;
	event.dmove.x = 400;
	event.dmove.y = 400;
	rc = ds_seat_post_ptd_event(seat, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	event.type = PTD_PRESS;
	event.btn_num = 1;
	rc = ds_seat_post_ptd_event(seat, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	PCUT_ASSERT_EQUALS(w1, seat->focus);

	event.type = PTD_MOVE;
	event.dmove.x = -400 + 10;
	event.dmove.y = -400 + 10;
	rc = ds_seat_post_ptd_event(seat, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	event.type = PTD_PRESS;
	event.btn_num = 1;
	rc = ds_seat_post_ptd_event(seat, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);
	called_cb = false;

	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_client_destroy(client);
	ds_seat_destroy(seat);
	ds_display_destroy(disp);
}

/** Test ds_seat_post_pos_event() */
PCUT_TEST(post_pos_event)
{
	// XXX
}

/** Set WM cursor */
PCUT_TEST(set_wm_cursor)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, "Alice", &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_wm_cursor(seat, disp->cursor[dcurs_size_ud]);
	ds_seat_set_wm_cursor(seat, NULL);

	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

PCUT_EXPORT(seat);
