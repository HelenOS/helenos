/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file User interface
 */

#ifndef _UI_TYPES_UI_H
#define _UI_TYPES_UI_H

#include <gfx/coord.h>

struct ui;
typedef struct ui ui_t;

/** Use the default display service (argument to ui_create()) */
#define UI_DISPLAY_DEFAULT "disp@"
/** Use the default console service (argument to ui_create()) */
#define UI_CONSOLE_DEFAULT "cons@"
/** Use any available service (argument to ui_create()) */
#define UI_ANY_DEFAULT "@"
/** Use dummy output (argument to ui_create()) */
#define UI_DISPLAY_NULL "null@"

/** Window system */
typedef enum {
	/** Unknown */
	ui_ws_unknown,
	/** Display service */
	ui_ws_display,
	/** Console */
	ui_ws_console,
	/** Any non-dummy output backend */
	ui_ws_any,
	/** Dummy output */
	ui_ws_null
} ui_winsys_t;

#endif

/** @}
 */
