/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator menu types
 */

#ifndef TYPES_MENU_H
#define TYPES_MENU_H

#include <ui/menubar.h>
#include <ui/ui.h>
#include <ui/window.h>

/** Navigator menu callbacks */
typedef struct nav_menu_cb {
	/** File / Open */
	void (*file_open)(void *);
	/** File / Exit */
	void (*file_exit)(void *);
} nav_menu_cb_t;

/** Navigator menu */
typedef struct nav_menu {
	/** UI */
	ui_t *ui;
	/** Containing window */
	ui_window_t *window;
	/** Menu bar */
	ui_menu_bar_t *menubar;
	/** Callbacks */
	nav_menu_cb_t *cb;
	/** Callback argument */
	void *cb_arg;
} nav_menu_t;

#endif

/** @}
 */
