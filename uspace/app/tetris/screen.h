/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 * SPDX-FileCopyrightText: 1992, 1993
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** Attributations
 *
 * screen.h 8.1 (Berkeley) 5/31/93
 * NetBSD: screen.h,v 1.2 1995/04/22 07:42:42 cgd
 * OpenBSD: screen.h,v 1.5 2003/06/03 03:01:41 millert
 *
 * Based upon BSD Tetris
 *
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

extern int put(int);
extern void scr_clear(void);
extern void scr_end(void);
extern void scr_init(void);
extern void scr_msg(char *, bool);
extern void scr_set(void);
extern void scr_update(void);

extern void tsleep(void);
extern int tgetchar(void);
extern int twait(void);

/** @}
 */
