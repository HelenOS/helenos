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

#include <sys/time.h>
#include <io/keycode.h>
#include <async.h>
#include <bool.h>
#include <stdio.h>

typedef enum {
	CONSOLE_CAP_NONE = 0,
	CONSOLE_CAP_STYLE = 1,
	CONSOLE_CAP_INDEXED = 2,
	CONSOLE_CAP_RGB = 4
} console_caps_t;

/** Console control structure. */
typedef struct {
	/** Console input file */
	FILE *input;
	
	/** Console output file */
	FILE *output;
	
	/** Console input session */
	async_sess_t *input_sess;
	
	/** Console output session */
	async_sess_t *output_sess;
	
	/** Input request call with timeout */
	ipc_call_t input_call;
	
	/** Input response with timeout */
	aid_t input_aid;
} console_ctrl_t;

typedef enum {
	KEY_PRESS,
	KEY_RELEASE
} kbd_event_type_t;

/** Console event structure. */
typedef struct {
	/** List handle */
	link_t link;
	
	/** Press or release event. */
	kbd_event_type_t type;
	
	/** Keycode of the key that was pressed or released. */
	keycode_t key;
	
	/** Bitmask of modifiers held. */
	keymod_t mods;
	
	/** The character that was generated or '\0' for none. */
	wchar_t c;
} kbd_event_t;

extern console_ctrl_t *console_init(FILE *, FILE *);
extern void console_done(console_ctrl_t *);
extern bool console_kcon(void);

extern void console_flush(console_ctrl_t *);
extern void console_clear(console_ctrl_t *);

extern int console_get_size(console_ctrl_t *, sysarg_t *, sysarg_t *);
extern int console_get_pos(console_ctrl_t *, sysarg_t *, sysarg_t *);
extern void console_set_pos(console_ctrl_t *, sysarg_t, sysarg_t);

extern void console_set_style(console_ctrl_t *, uint8_t);
extern void console_set_color(console_ctrl_t *, uint8_t, uint8_t, uint8_t);
extern void console_set_rgb_color(console_ctrl_t *, uint32_t, uint32_t);

extern void console_cursor_visibility(console_ctrl_t *, bool);
extern int console_get_color_cap(console_ctrl_t *, sysarg_t *);
extern bool console_get_kbd_event(console_ctrl_t *, kbd_event_t *);
extern bool console_get_kbd_event_timeout(console_ctrl_t *, kbd_event_t *,
    suseconds_t *);

#endif

/** @}
 */
