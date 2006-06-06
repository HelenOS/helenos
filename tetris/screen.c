/*	$OpenBSD: screen.c,v 1.13 2006/04/20 03:25:36 ray Exp $	*/
/*	$NetBSD: screen.c,v 1.4 1995/04/29 01:11:36 mycroft Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)screen.c	8.1 (Berkeley) 5/31/93
 */

/*
 * Tetris screen control.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <io/stream.h>


#include <async.h>
#include "screen.h"
#include "tetris.h"
#include "../console/console.h"

static cell curscreen[B_SIZE];	/* 1 => standout (or otherwise marked) */
static int curscore;
static int isset;		/* true => terminal is in game mode */
static void (*tstp)(int);

static void	scr_stop(int);
static void	stopset(int);

static char
        *CEstr;			/* clear to end of line */


/*
 * putstr() is for unpadded strings (either as in termcap(5) or
 * simply literal strings); 
 */
static inline void putstr(char *s)
{
	while (*s)
		putchar(*(s++));
}

static int con_phone;



static void set_style(int fgcolor, int bgcolor)
{
	async_msg_2(con_phone, CONSOLE_SET_STYLE, fgcolor, bgcolor);
}

static void start_standout(void)
{
	set_style(0, 0xe0e0e0);
}

static void resume_normal(void)
{
	set_style(0xe0e0e0, 0);
}


void clear_screen(void)
{
	async_msg(con_phone, CONSOLE_CLEAR, 0);
	moveto(0,0);
}

/*
 * Clear the screen, forgetting the current contents in the process.
 */
void
scr_clear(void)
{

	resume_normal();
	async_msg(con_phone, CONSOLE_CLEAR, 0);
	curscore = -1;
	memset((char *)curscreen, 0, sizeof(curscreen));
}

/*
 * Set up screen
 */
void
scr_init(void)
{
	con_phone = get_fd_phone(1);
	async_msg(con_phone, CONSOLE_CURSOR_VISIBILITY, 0);
	resume_normal();
	scr_clear();
}

void moveto(int r, int c)
{
	async_msg_2(con_phone, CONSOLE_GOTO, r, c);
}

static void fflush(void)
{
	async_msg(con_phone, CONSOLE_FLUSH, 0);
}

winsize_t winsize;

static int get_display_size(winsize_t *ws)
{
	return async_req_2(con_phone, CONSOLE_GETSIZE, 0, 0, &ws->ws_row, &ws->ws_col);
}

static void
scr_stop(int sig)
{

	scr_end();
	scr_set();
	scr_msg(key_msg, 1);
}

/*
 * Set up screen mode.
 */
void
scr_set(void)
{
	winsize_t ws;

	Rows = 0, Cols = 0;
	if (get_display_size(&ws) == 0) {
		Rows = ws.ws_row;
		Cols = ws.ws_col;
	}
	if (Rows < MINROWS || Cols < MINCOLS) {
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
void
scr_end(void)
{
}

void
stop(char *why)
{

	if (isset)
		scr_end();
	errx(1, "aborting: %s", why);
}


/*
 * Update the screen.
 */
void
scr_update(void)
{
	cell *bp, *sp;
	cell so, cur_so = 0;
	int i, ccol, j;
	static const struct shape *lastshape;

	/* always leave cursor after last displayed point */
	curscreen[D_LAST * B_COLS - 1] = -1;

	if (score != curscore) {
		moveto(0, 0);
		printf("Score: %d", score);
		curscore = score;
	}

	/* draw preview of next pattern */
	if (showpreview && (nextshape != lastshape)) {
		int i;
		static int r=5, c=2;
		int tr, tc, t;

		lastshape = nextshape;

		/* clean */
		resume_normal();
		moveto(r-1, c-1); putstr("          ");
		moveto(r,   c-1); putstr("          ");
		moveto(r+1, c-1); putstr("          ");
		moveto(r+2, c-1); putstr("          ");

		moveto(r-3, c-2);
		putstr("Next shape:");

		/* draw */
		start_standout();
		moveto(r, 2 * c);
		putstr("  ");
		for (i = 0; i < 3; i++) {
			t = c + r * B_COLS;
			t += nextshape->off[i];

			tr = t / B_COLS;
			tc = t % B_COLS;

			moveto(tr, 2*tc);
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
					start_standout();
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
#define	STOP (B_COLS - 3)
			if (i > STOP || sp[1] != bp[1] || so != bp[1])
				continue;
			if (sp[2] != bp[2])
				sp[1] = -1;
			else if (i < STOP && so == bp[2] && sp[3] != bp[3]) {
				sp[2] = -1;
				sp[1] = -1;
			}
		}
	}
	if (cur_so)
		resume_normal();
 	fflush();
}

/*
 * Write a message (set!=0), or clear the same message (set==0).
 * (We need its length in case we have to overwrite with blanks.)
 */
void
scr_msg(char *s, int set)
{
	
	int l = strlen(s);
	
	moveto(Rows - 2, ((Cols - l) >> 1) - 1);
	if (set)
		putstr(s);
	else
		while (--l >= 0)
			(void) putchar(' ');
}
