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
 * screen.c 8.1 (Berkeley) 5/31/93
 * NetBSD: screen.c,v 1.4 1995/04/29 01:11:36 mycroft
 * OpenBSD: screen.c,v 1.13 2006/04/20 03:25:36 ray
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
 * Tetris screen control.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <vfs/vfs.h>
#include <async.h>
#include <stdbool.h>
#include <io/console.h>
#include <io/style.h>
#include "screen.h"
#include "tetris.h"

#define STOP  (B_COLS - 3)

static cell curscreen[B_SIZE];  /* non-zero => standout (or otherwise marked) */
static int curscore;
static int isset;               /* true => terminal is in game mode */

static bool use_color;          /* true => use colors */

static const struct shape *lastshape;

static suseconds_t timeleft = 0;

console_ctrl_t *console;

/*
 * putstr() is for unpadded strings (either as in termcap(5) or
 * simply literal strings);
 */
static inline void putstr(const char *s)
{
	while (*s)
		putchar(*(s++));
}

static void start_standout(uint32_t color)
{
	console_flush(console);
	console_set_rgb_color(console, use_color ? color : 0x000000,
	    0xffffff);
}

static void resume_normal(void)
{
	console_flush(console);
	console_set_style(console, STYLE_NORMAL);
}

void clear_screen(void)
{
	console_clear(console);
	moveto(0, 0);
}

/*
 * Clear the screen, forgetting the current contents in the process.
 */
void scr_clear(void)
{
	resume_normal();
	console_clear(console);
	curscore = -1;
	memset(curscreen, 0, sizeof(curscreen));
}

/*
 * Set up screen
 */
void scr_init(void)
{
	console_cursor_visibility(console, 0);
	resume_normal();
	scr_clear();
}

void moveto(sysarg_t r, sysarg_t c)
{
	console_flush(console);
	console_set_pos(console, c, r);
}

winsize_t winsize;

static int get_display_size(winsize_t *ws)
{
	return console_get_size(console, &ws->ws_col, &ws->ws_row);
}

static bool get_display_color_sup(void)
{
	sysarg_t ccap;
	errno_t rc = console_get_color_cap(console, &ccap);

	if (rc != EOK)
		return false;

	return ((ccap & CONSOLE_CAP_RGB) == CONSOLE_CAP_RGB);
}

/*
 * Set up screen mode.
 */
void scr_set(void)
{
	winsize_t ws;

	Rows = 0;
	Cols = 0;

	if (get_display_size(&ws) == 0) {
		Rows = ws.ws_row;
		Cols = ws.ws_col;
	}

	use_color = get_display_color_sup();

	if ((Rows < MINROWS) || (Cols < MINCOLS)) {
		char smallscr[55];

		snprintf(smallscr, sizeof(smallscr),
		    "the screen is too small (must be at least %dx%d)",
		    MINROWS, MINCOLS);
		stop(smallscr);
	}
	isset = 1;

	scr_clear();
}

/*
 * End screen mode.
 */
void scr_end(void)
{
	console_cursor_visibility(console, 1);
}

void stop(const char *why)
{
	if (isset)
		scr_end();

	errx(1, "aborting: %s", why);
}

/*
 * Update the screen.
 */
void scr_update(void)
{
	cell *bp;
	cell *sp;
	cell so;
	cell cur_so = 0;
	int i;
	int j;
	int ccol;

	/* Always leave cursor after last displayed point */
	curscreen[D_LAST * B_COLS - 1] = -1;

	if (score != curscore) {
		moveto(0, 0);
		printf("Score: %d", score);
		curscore = score;
	}

	/* Draw preview of next pattern */
	if ((showpreview) && (nextshape != lastshape)) {
		int i;
		static int r = 5, c = 2;
		int tr, tc, t;

		lastshape = nextshape;

		/* Clean */
		resume_normal();
		moveto(r - 1, c - 1);
		putstr("          ");
		moveto(r, c - 1);
		putstr("          ");
		moveto(r + 1, c - 1);
		putstr("          ");
		moveto(r + 2, c - 1);
		putstr("          ");

		moveto(r - 3, c - 2);
		putstr("Next shape:");

		/* Draw */
		start_standout(nextshape->color);
		moveto(r, 2 * c);
		putstr("  ");
		for (i = 0; i < 3; i++) {
			t = c + r * B_COLS;
			t += nextshape->off[i];

			tr = t / B_COLS;
			tc = t % B_COLS;

			moveto(tr, 2 * tc);
			putstr("  ");
		}
		resume_normal();
	}

	bp = &board[D_FIRST * B_COLS];
	sp = &curscreen[D_FIRST * B_COLS];
	for (j = D_FIRST; j < D_LAST; j++) {
		ccol = -1;
		for (i = 0; i < B_COLS; bp++, sp++, i++) {
			if (*sp == (so = *bp))
				continue;

			*sp = so;
			if (i != ccol) {
				if (cur_so) {
					resume_normal();
					cur_so = 0;
				}
				moveto(RTOD(j), CTOD(i));
			}

			if (so != cur_so) {
				if (so)
					start_standout(so);
				else
					resume_normal();
				cur_so = so;
			}
			putstr("  ");

			ccol = i + 1;
			/*
			 * Look ahead a bit, to avoid extra motion if
			 * we will be redrawing the cell after the next.
			 * Motion probably takes four or more characters,
			 * so we save even if we rewrite two cells
			 * `unnecessarily'.  Skip it all, though, if
			 * the next cell is a different color.
			 */

			if ((i > STOP) || (sp[1] != bp[1]) || (so != bp[1]))
				continue;

			if (sp[2] != bp[2])
				sp[1] = -1;
			else if ((i < STOP) && (so == bp[2]) && (sp[3] != bp[3])) {
				sp[2] = -1;
				sp[1] = -1;
			}
		}
	}

	if (cur_so)
		resume_normal();

	console_flush(console);
}

/*
 * Write a message (set != 0), or clear the same message (set == 0).
 * (We need its length in case we have to overwrite with blanks.)
 */
void scr_msg(char *s, bool set)
{
	int l = str_size(s);

	moveto(Rows - 2, ((Cols - l) >> 1) - 1);

	if (set)
		putstr(s);
	else
		while (--l >= 0)
			(void) putchar(' ');
}

/** Sleep for the current turn time
 *
 * Eat any input that might be available.
 *
 */
void tsleep(void)
{
	suseconds_t timeout = fallrate;

	while (timeout > 0) {
		cons_event_t event;

		if (!console_get_event_timeout(console, &event, &timeout))
			break;
	}
}

/** Get char with timeout
 *
 */
errno_t tgetchar(void)
{
	/*
	 * Reset timeleft to fallrate whenever it is not positive
	 * and increase speed.
	 */

	if (timeleft <= 0) {
		faster();
		timeleft = fallrate;
	}

	/*
	 * Wait to see if there is any input. If so, take it and
	 * update timeleft so that the next call to tgetchar()
	 * will not wait as long. If there is no input,
	 * make timeleft zero and return -1.
	 */

	char32_t c = 0;

	while (c == 0) {
		cons_event_t event;

		if (!console_get_event_timeout(console, &event, &timeleft)) {
			timeleft = 0;
			return -1;
		}

		if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS)
			c = event.ev.key.c;
	}

	return (int) c;
}

/** Get char without timeout
 *
 */
errno_t twait(void)
{
	char32_t c = 0;

	while (c == 0) {
		cons_event_t event;

		if (!console_get_event(console, &event))
			return -1;

		if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS)
			c = event.ev.key.c;
	}

	return (int) c;
}

/** @}
 */
