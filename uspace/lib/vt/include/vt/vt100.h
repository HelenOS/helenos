/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup libvt
 * @{
 */

#ifndef LIBVT_VT100_H_
#define LIBVT_VT100_H_

#include <io/charfield.h>
#include <ipc/common.h>
#include <uchar.h>

/** Buffer size when creating actual VT100 commands.
 *
 * This is absurdly large but since we accept numbers via sysarg_t,
 * we make it big enough for the largest value to be on the safe side
 * (and to silence compiler too).
 *
 * TODO: find out if VT100 has some hard limits or perhaps simply cut-out
 * values larger than 16 bits or something.
 */
#define MAX_CONTROL 64

typedef enum {
	CI_BLACK   = 0,
	CI_RED     = 1,
	CI_GREEN   = 2,
	CI_BROWN   = 3,
	CI_BLUE    = 4,
	CI_MAGENTA = 5,
	CI_CYAN    = 6,
	CI_WHITE   = 7
} sgr_color_index_t;

typedef enum {
	SGR_RESET       = 0,
	SGR_BOLD        = 1,
	SGR_UNDERLINE   = 4,
	SGR_BLINK       = 5,
	SGR_REVERSE     = 7,
	SGR_FGCOLOR     = 30,
	SGR_BGCOLOR     = 40
} sgr_command_t;

typedef void (*vt100_putuchar_t)(void *, char32_t ch);
typedef void (*vt100_control_puts_t)(void *, const char *str);
typedef void (*vt100_flush_t)(void *);

typedef struct {
	sysarg_t cols;
	sysarg_t rows;

	sysarg_t cur_col;
	sysarg_t cur_row;
	char_attrs_t cur_attrs;

	bool enable_rgb;

	void *arg;
	vt100_putuchar_t putuchar;
	vt100_control_puts_t control_puts;
	vt100_flush_t flush;
} vt100_state_t;

extern sgr_color_index_t color_map[];

extern vt100_state_t *vt100_state_create(void *, sysarg_t, sysarg_t,
    vt100_putuchar_t, vt100_control_puts_t, vt100_flush_t);
extern void vt100_state_destroy(vt100_state_t *);

extern errno_t vt100_yield(vt100_state_t *);
extern errno_t vt100_claim(vt100_state_t *);
extern void vt100_get_dimensions(vt100_state_t *, sysarg_t *, sysarg_t *);

extern void vt100_cls(vt100_state_t *);
extern void vt100_set_pos(vt100_state_t *, sysarg_t, sysarg_t);
extern void vt100_goto(vt100_state_t *, sysarg_t, sysarg_t);
extern void vt100_set_sgr(vt100_state_t *, char_attrs_t);
extern void vt100_set_attr(vt100_state_t *, char_attrs_t);
extern void vt100_cursor_visibility(vt100_state_t *, bool);
extern void vt100_putuchar(vt100_state_t *, char32_t);
extern void vt100_flush(vt100_state_t *);

#endif

/** @}
 */
