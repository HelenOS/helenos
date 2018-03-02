/*
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

/** @file
 */

#ifndef OUTPUT_PROTO_VT100_H_
#define OUTPUT_PROTO_VT100_H_

#include <io/charfield.h>

typedef void (*vt100_putchar_t)(wchar_t ch);
typedef void (*vt100_control_puts_t)(const char *str);
typedef void (*vt100_flush_t)(void);

typedef struct {
	sysarg_t cols;
	sysarg_t rows;

	sysarg_t cur_col;
	sysarg_t cur_row;
	char_attrs_t cur_attrs;

	vt100_putchar_t putchar;
	vt100_control_puts_t control_puts;
	vt100_flush_t flush;
} vt100_state_t;

extern vt100_state_t *vt100_state_create(sysarg_t, sysarg_t, vt100_putchar_t,
    vt100_control_puts_t, vt100_flush_t);
extern void vt100_state_destroy(vt100_state_t *);

extern errno_t vt100_yield(vt100_state_t *);
extern errno_t vt100_claim(vt100_state_t *);
extern void vt100_get_dimensions(vt100_state_t *, sysarg_t *, sysarg_t *);

extern void vt100_goto(vt100_state_t *, sysarg_t, sysarg_t);
extern void vt100_set_attr(vt100_state_t *, char_attrs_t);
extern void vt100_cursor_visibility(vt100_state_t *, bool);
extern void vt100_putchar(vt100_state_t *, wchar_t);
extern void vt100_flush(vt100_state_t *);

#endif

/** @}
 */
