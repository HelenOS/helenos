/*
 * Copyright (c) 2021 Jiri Svoboda
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
