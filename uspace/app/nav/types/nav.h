/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator types
 */

#ifndef TYPES_NAV_H
#define TYPES_NAV_H

#include <ui/fixed.h>
#include <ui/ui.h>
#include <ui/window.h>

enum {
	navigator_panels = 2
};

/** Navigator */
typedef struct navigator {
	/** User interface */
	ui_t *ui;
	/** Window */
	ui_window_t *window;
	/** Fixed layout */
	ui_fixed_t *fixed;
	/** Menu */
	struct nav_menu *menu;
	/** Panels */
	struct panel *panel[navigator_panels];
} navigator_t;

#endif

/** @}
 */
