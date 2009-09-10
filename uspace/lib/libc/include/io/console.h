/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_IO_CONSOLE_H_
#define LIBC_IO_CONSOLE_H_

#include <ipc/ipc.h>
#include <bool.h>

typedef enum {
	KEY_PRESS,
	KEY_RELEASE
} console_ev_type_t;

enum {
	CONSOLE_CCAP_NONE = 0,
	CONSOLE_CCAP_STYLE,
	CONSOLE_CCAP_INDEXED,
	CONSOLE_CCAP_RGB
};

/** Console event structure. */
typedef struct {
	/** Press or release event. */
	console_ev_type_t type;
	
	/** Keycode of the key that was pressed or released. */
	unsigned int key;
	
	/** Bitmask of modifiers held. */
	unsigned int mods;
	
	/** The character that was generated or '\0' for none. */
	wchar_t c;
} console_event_t;

extern void console_clear(int phone);

extern int console_get_size(int phone, int *cols, int *rows);
extern void console_goto(int phone, int col, int row);

extern void console_set_style(int phone, int style);
extern void console_set_color(int phone, int fg_color, int bg_color, int flags);
extern void console_set_rgb_color(int phone, int fg_color, int bg_color);

extern void console_cursor_visibility(int phone, bool show);
extern int console_get_color_cap(int phone, int *ccap);
extern void console_kcon_enable(int phone);

extern bool console_get_event(int phone, console_event_t *event);

#endif

/** @}
 */
