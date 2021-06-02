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
#include <str.h>

#include "../client.h"
#include "../display.h"
#include "../seat.h"
#include "../window.h"

PCUT_INIT;

PCUT_TEST_SUITE(display);

static void test_ds_ev_pending(void *);

static ds_client_cb_t test_ds_client_cb = {
	.ev_pending = test_ds_ev_pending
};

static void test_ds_ev_pending(void *arg)
{
	bool *called_cb = (bool *) arg;
	*called_cb = true;
}

/** Display creation and destruction. */
PCUT_TEST(display_create_destroy)
{
	ds_display_t *disp;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_display_destroy(disp);
}

/** Basic client operation. */
PCUT_TEST(display_client)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_client_t *c0, *c1;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	c0 = ds_display_first_client(disp);
	PCUT_ASSERT_EQUALS(c0, client);

	c1 = ds_display_next_client(c0);
	PCUT_ASSERT_NULL(c1);

	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_display_find_window(). */
PCUT_TEST(display_find_window)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	ds_window_t *wnd;
	display_wnd_params_t params;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
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

	wnd = ds_display_first_window(disp);
	PCUT_ASSERT_EQUALS(w0, wnd);

	wnd = ds_display_next_window(wnd);
	PCUT_ASSERT_EQUALS(w1, wnd);

	wnd = ds_display_next_window(wnd);
	PCUT_ASSERT_NULL(wnd);

	wnd = ds_display_last_window(disp);
	PCUT_ASSERT_EQUALS(w1, wnd);

	wnd = ds_display_prev_window(wnd);
	PCUT_ASSERT_EQUALS(w0, wnd);

	wnd = ds_display_prev_window(wnd);
	PCUT_ASSERT_NULL(wnd);

	wnd = ds_display_find_window(disp, w0->id);
	PCUT_ASSERT_EQUALS(w0, wnd);

	wnd = ds_display_find_window(disp, w1->id);
	PCUT_ASSERT_EQUALS(w1, wnd);

	wnd = ds_display_find_window(disp, 0);
	PCUT_ASSERT_NULL(wnd);

	wnd = ds_display_find_window(disp, w0->id + 1);
	PCUT_ASSERT_NULL(wnd);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_display_window_by_pos(). */
PCUT_TEST(display_window_by_pos)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_seat_t *seat;
	ds_window_t *w0;
	ds_window_t *w1;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_coord2_t pos;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 100;

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	w0->dpos.x = 10;
	w0->dpos.y = 10;

	w1->dpos.x = 400;
	w1->dpos.y = 400;

	pos.x = 10;
	pos.y = 10;
	wnd = ds_display_window_by_pos(disp, &pos);
	PCUT_ASSERT_EQUALS(w0, wnd);

	pos.x = 400;
	pos.y = 400;
	wnd = ds_display_window_by_pos(disp, &pos);
	PCUT_ASSERT_EQUALS(w1, wnd);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_seat_destroy(seat);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Basic seat operation. */
PCUT_TEST(display_seat)
{
	ds_display_t *disp;
	ds_seat_t *seat;
	ds_seat_t *s0, *s1;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	s0 = ds_display_first_seat(disp);
	PCUT_ASSERT_EQUALS(s0, seat);

	s1 = ds_display_next_seat(s0);
	PCUT_ASSERT_NULL(s1);

	ds_seat_destroy(seat);
	ds_display_destroy(disp);
}

/** Test ds_display_post_kbd_event() delivers event to client callback.
 */
PCUT_TEST(display_post_kbd_event)
{
	ds_display_t *disp;
	ds_seat_t *seat;
	ds_client_t *client;
	ds_window_t *wnd;
	display_wnd_params_t params;
	kbd_event_t event;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, wnd);

	event.type = KEY_PRESS;
	event.key = KC_ENTER;
	event.mods = 0;
	event.c = L'\0';

	called_cb = false;

	rc = ds_display_post_kbd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(called_cb);

	ds_window_destroy(wnd);
	ds_client_destroy(client);
	ds_seat_destroy(seat);
	ds_display_destroy(disp);
}

/** Test ds_display_post_kbd_event() with Alt-Tab switches focus.
 */
PCUT_TEST(display_post_kbd_event_alt_tab)
{
	ds_display_t *disp;
	ds_seat_t *seat;
	ds_client_t *client;
	ds_window_t *w0, *w1;
	display_wnd_params_t params;
	kbd_event_t event;
	bool called_cb = false;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_seat_create(disp, &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &w0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_window_create(client, &params, &w1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_seat_set_focus(seat, w0);

	event.type = KEY_PRESS;
	event.key = KC_TAB;
	event.mods = KM_ALT;
	event.c = L'\0';

	called_cb = false;

	rc = ds_display_post_kbd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Got gocus/unfocus events */
	PCUT_ASSERT_TRUE(called_cb);

	/* Next window should be focused */
	PCUT_ASSERT_EQUALS(w1, seat->focus);

	called_cb = false;

	rc = ds_display_post_kbd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Got gocus/unfocus events */
	PCUT_ASSERT_TRUE(called_cb);

	/* Focus should be back to the first window */
	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_client_destroy(client);
	ds_seat_destroy(seat);
	ds_display_destroy(disp);
}

/** Test ds_display_post_ptd_event() with click on window switches focus
 */
PCUT_TEST(display_post_ptd_event_wnd_switch)
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

	rc = ds_seat_create(disp, &seat);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, &test_ds_client_cb, &called_cb, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/*
	 * For PTD_MOVE to work we need to set display dimensions (as pointer
	 * move is clipped to the display rectangle. Here we do it directly
	 * instead of adding a display device.
	 */
	disp->rect.p0.x = 0;
	disp->rect.p0.y = 0;
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

	ds_seat_set_focus(seat, w0);

	event.type = PTD_MOVE;
	event.dmove.x = 400;
	event.dmove.y = 400;
	rc = ds_display_post_ptd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = PTD_PRESS;
	event.btn_num = 1;
	rc = ds_display_post_ptd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(w1, seat->focus);

	event.type = PTD_RELEASE;
	event.btn_num = 1;
	rc = ds_display_post_ptd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = PTD_MOVE;
	event.dmove.x = -400 + 10;
	event.dmove.y = -400 + 10;
	rc = ds_display_post_ptd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = PTD_PRESS;
	event.btn_num = 1;
	rc = ds_display_post_ptd_event(disp, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS(w0, seat->focus);

	ds_window_destroy(w0);
	ds_window_destroy(w1);
	ds_client_destroy(client);
	ds_seat_destroy(seat);
	ds_display_destroy(disp);
}

PCUT_EXPORT(display);
