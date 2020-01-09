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
#include <gfx/context.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <str.h>

#include "../client.h"
#include "../display.h"
#include "../window.h"

PCUT_INIT;

PCUT_TEST_SUITE(window);

static errno_t dummy_set_color(void *, gfx_color_t *);
static errno_t dummy_fill_rect(void *, gfx_rect_t *);

static gfx_context_ops_t dummy_ops = {
	.set_color = dummy_set_color,
	.fill_rect = dummy_fill_rect
};

/** Test ds_window_get_ctx(). */
PCUT_TEST(window_get_ctx)
{
	ds_display_t *disp;
	ds_client_t *client;
	ds_window_t *wnd;
	display_wnd_params_t params;
	gfx_context_t *gc;
	errno_t rc;

	rc = ds_display_create(NULL, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_window_get_ctx(wnd);
	PCUT_ASSERT_NOT_NULL(gc);

	ds_window_destroy(wnd);
	ds_client_destroy(client);
	ds_display_destroy(disp);
}

/** Test ds_window_post_pos_event() */
PCUT_TEST(window_post_pos_event)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_client_t *client;
	ds_window_t *wnd;
	display_wnd_params_t params;
	pos_event_t event;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_client_create(disp, NULL, NULL, &client);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_wnd_params_init(&params);
	params.rect.p0.x = params.rect.p0.y = 0;
	params.rect.p1.x = params.rect.p1.y = 1;

	rc = ds_window_create(client, &params, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_idle, wnd->state);

	event.type = POS_PRESS;
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
	PCUT_ASSERT_INT_EQUALS(wnd->dpos.x, 0);
	PCUT_ASSERT_INT_EQUALS(wnd->dpos.y, 0);

	event.type = POS_RELEASE;
	event.hpos = 13;
	event.vpos = 14;

	rc = ds_window_post_pos_event(wnd, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(dsw_idle, wnd->state);
	PCUT_ASSERT_INT_EQUALS(wnd->dpos.x, 3);
	PCUT_ASSERT_INT_EQUALS(wnd->dpos.y, 4);

	ds_window_destroy(wnd);
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
