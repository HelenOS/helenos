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

/** Attributations
 *
 * screen.h 8.1 (Berkeley) 5/31/93
 * NetBSD: screen.h,v 1.2 1995/04/22 07:42:42 cgd
 * OpenBSD: screen.h,v 1.5 2003/06/03 03:01:41 millert
 *
 * Based upon BSD Tetris
 *
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.
 *      Distributed under BSD license.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
 *
 */

/** @addtogroup tetris
 * @{
 */
/** @file
 */

/*
 * putpad() is for padded strings with count = 1.
 */
#define putpad(s)  tputs(s, 1, put)

#include <types/common.h>
#include <io/console.h>
#include <async.h>
#include <stdbool.h>

typedef struct {
	sysarg_t ws_row;
	sysarg_t ws_col;
} winsize_t;

extern console_ctrl_t *console;
extern winsize_t winsize;

extern void moveto(sysarg_t r, sysarg_t c);
extern void clear_screen(void);

extern errno_t put(int);
extern void scr_clear(void);
extern void scr_end(void);
extern void scr_init(void);
extern void scr_msg(char *, bool);
extern void scr_set(void);
extern void scr_update(void);

extern void tsleep(void);
extern errno_t tgetchar(void);
extern errno_t twait(void);

/** @}
 */
