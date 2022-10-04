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

#include <errno.h>
#include <gfx/coord.h>
#include <pcut/pcut.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../clock.h"

PCUT_INIT;

PCUT_TEST_SUITE(clock);

/** Creating and destroying taskbar clock */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	taskbar_clock_t *clock;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = taskbar_clock_create(window, &clock);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	taskbar_clock_destroy(clock);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Painting taskbar clock */
PCUT_TEST(paint)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	taskbar_clock_t *clock;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = taskbar_clock_create(window, &clock);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = taskbar_clock_paint(clock);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	taskbar_clock_destroy(clock);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Keyboard event */
PCUT_TEST(kbd_event)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	taskbar_clock_t *clock;
	kbd_event_t event;
	ui_evclaim_t claim;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = taskbar_clock_create(window, &clock);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ENTER;
	event.mods = 0;
	claim = taskbar_clock_kbd_event(clock, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claim);

	taskbar_clock_destroy(clock);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Position event */
PCUT_TEST(pos_event)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	taskbar_clock_t *clock;
	pos_event_t event;
	ui_evclaim_t claim;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = taskbar_clock_create(window, &clock);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = POS_PRESS;
	event.hpos = 0;
	event.vpos = 0;
	claim = taskbar_clock_pos_event(clock, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claim);

	taskbar_clock_destroy(clock);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** taskbar_clock_ctl returns a usable UI control */
PCUT_TEST(ctl)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	taskbar_clock_t *clock;
	ui_control_t *ctl;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = taskbar_clock_create(window, &clock);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ctl = taskbar_clock_ctl(clock);
	PCUT_ASSERT_NOT_NULL(ctl);

	rc = ui_control_paint(ctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	taskbar_clock_destroy(clock);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** taskbar_clock_set_rect() sets internal field */
PCUT_TEST(set_rect)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	taskbar_clock_t *clock;
	gfx_rect_t rect;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = taskbar_clock_create(window, &clock);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;
	taskbar_clock_set_rect(clock, &rect);

	PCUT_ASSERT_INT_EQUALS(rect.p0.x, clock->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, clock->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, clock->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, clock->rect.p1.y);

	taskbar_clock_destroy(clock);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(clock);
