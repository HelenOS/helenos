/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <ui/ui.h>
#include <vfs/vfs.h>
#include "../panel.h"

PCUT_INIT;

PCUT_TEST_SUITE(panel);

/** Test response */
typedef struct {
	bool activate_req;
	panel_t *activate_req_panel;
} test_resp_t;

static void test_panel_activate_req(void *, panel_t *);

static panel_cb_t test_cb = {
	.activate_req = test_panel_activate_req
};

/** Create and destroy panel. */
PCUT_TEST(create_destroy)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_destroy(panel);
}

/** panel_set_cb() sets callback */
PCUT_TEST(set_cb)
{
	panel_t *panel;
	errno_t rc;
	test_resp_t resp;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_set_cb(panel, &test_cb, &resp);
	PCUT_ASSERT_EQUALS(&test_cb, panel->cb);
	PCUT_ASSERT_EQUALS(&resp, panel->cb_arg);

	panel_destroy(panel);
}

/** Test panel_paint() */
PCUT_TEST(paint)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_paint(panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_ctl() returns a valid UI control */
PCUT_TEST(ctl)
{
	panel_t *panel;
	ui_control_t *control;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = panel_ctl(panel);
	PCUT_ASSERT_NOT_NULL(control);

	panel_destroy(panel);
}

/** Test panel_kbd_event() */
PCUT_TEST(kbd_event)
{
	panel_t *panel;
	ui_evclaim_t claimed;
	kbd_event_t event;
	errno_t rc;

	/* Active panel should claim events */

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = panel_kbd_event(panel, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	panel_destroy(panel);

	/* Inactive panel should not claim events */

	rc = panel_create(NULL, false, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = panel_kbd_event(panel, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	panel_destroy(panel);
}

/** Test panel_pos_event() */
PCUT_TEST(pos_event)
{
}

/** panel_set_rect() sets internal field */
PCUT_TEST(set_rect)
{
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	panel_set_rect(panel, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, panel->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, panel->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, panel->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, panel->rect.p1.y);

	panel_destroy(panel);
}

/** panel_is_active() returns panel activity state */
PCUT_TEST(is_active)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(panel_is_active(panel));
	panel_destroy(panel);

	rc = panel_create(NULL, false, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(panel_is_active(panel));
	panel_destroy(panel);
}

/** panel_activate() activates panel */
PCUT_TEST(activate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, false, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(panel_is_active(panel));
	rc = panel_activate(panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(panel_is_active(panel));

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_deactivate() deactivates panel */
PCUT_TEST(deactivate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(panel_is_active(panel));
	panel_deactivate(panel);
	PCUT_ASSERT_FALSE(panel_is_active(panel));

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_activate_req() sends activation request */
PCUT_TEST(activate_req)
{
	panel_t *panel;
	errno_t rc;
	test_resp_t resp;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_set_cb(panel, &test_cb, &resp);

	resp.activate_req = false;
	resp.activate_req_panel = NULL;

	panel_activate_req(panel);
	PCUT_ASSERT_TRUE(resp.activate_req);
	PCUT_ASSERT_EQUALS(panel, resp.activate_req_panel);

	panel_destroy(panel);
}

static void test_panel_activate_req(void *arg, panel_t *panel)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->activate_req = true;
	resp->activate_req_panel = panel;
}

PCUT_EXPORT(panel);
