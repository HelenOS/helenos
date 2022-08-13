/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <pcut/pcut.h>
#include "../nav.h"

PCUT_INIT;

PCUT_TEST_SUITE(nav);

/** Create and destroy navigator. */
PCUT_TEST(create_destroy)
{
	navigator_t *nav;
	errno_t rc;

	rc = navigator_create(UI_DISPLAY_NULL, &nav);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	navigator_destroy(nav);
}

/** navigator_get_active_panel() returns the active panel */
PCUT_TEST(get_active_panel)
{
	navigator_t *nav;
	panel_t *panel;
	errno_t rc;

	rc = navigator_create(UI_DISPLAY_NULL, &nav);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* First panel should be active at the beginning */
	panel = navigator_get_active_panel(nav);
	PCUT_ASSERT_EQUALS(nav->panel[0], panel);

	navigator_destroy(nav);
}

/** navigator_switch_panel() switches to a different panel */
PCUT_TEST(switch_panel)
{
	navigator_t *nav;
	panel_t *panel;
	errno_t rc;

	rc = navigator_create(UI_DISPLAY_NULL, &nav);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* First panel should be active at the beginning */
	panel = navigator_get_active_panel(nav);
	PCUT_ASSERT_EQUALS(nav->panel[0], panel);

	navigator_switch_panel(nav);

	/* Second panel should be active now */
	panel = navigator_get_active_panel(nav);
	PCUT_ASSERT_EQUALS(nav->panel[1], panel);

	navigator_switch_panel(nav);

	/* First panel should be active again */
	panel = navigator_get_active_panel(nav);
	PCUT_ASSERT_EQUALS(nav->panel[0], panel);

	navigator_destroy(nav);
}

PCUT_EXPORT(nav);
