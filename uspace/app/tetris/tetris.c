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
 * tetris.c 8.1 (Berkeley) 5/31/93
 * NetBSD: tetris.c,v 1.2 1995/04/22 07:42:47 cgd
 * OpenBSD: tetris.c,v 1.21 2006/04/20 03:24:12 ray
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

static volatile const char copyright[] =
    "@(#) Copyright (c) 1992, 1993\n"
    "\tThe Regents of the University of California.  All rights reserved.\n";

#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <str.h>
#include <getopt.h>
#include "scores.h"
#include "screen.h"
#include "tetris.h"

cell board[B_SIZE];

int Rows;
int Cols;

const struct shape *curshape;
const struct shape *nextshape;

long fallrate;
int score;
char key_msg[116];
int showpreview;
int classic;

static void elide(void);
static void setup_board(void);
static const struct shape *randshape(void);

static void usage(void);

static int firstgame = 1;

/*
 * Set up the initial board. The bottom display row is completely set,
 * along with another (hidden) row underneath that. Also, the left and
 * right edges are set.
 */
static void setup_board(void)
{
	int i;
	cell *p = board;

	for (i = B_SIZE; i; i--)
		*p++ = (i <= (2 * B_COLS) || (i % B_COLS) < 2) ? 0x0000ff : 0x000000;
}

/*
 * Elide any full active rows.
 */
static void elide(void)
{
	int rows = 0;
	int i;
	int j;
	int base;
	cell *p;

	for (i = A_FIRST; i < A_LAST; i++) {
		base = i * B_COLS + 1;
		p = &board[base];
		j = B_COLS - 2;
		while (*p++ != 0) {
			if (--j <= 0) {
				/* This row is to be elided */
				rows++;
				memset(&board[base], 0, sizeof(cell) * (B_COLS - 2));

				scr_update();
				tsleep();

				while (--base != 0)
					board[base + B_COLS] = board[base];

				scr_update();
				tsleep();

				break;
			}
		}
	}

	switch (rows) {
	case 1:
		score += 10;
		break;
	case 2:
		score += 30;
		break;
	case 3:
		score += 70;
		break;
	case 4:
		score += 150;
		break;
	default:
		break;
	}
}

const struct shape *randshape(void)
{
	const struct shape *tmp = &shapes[rand() % 7];
	int i;
	int j = rand() % 4;

	for (i = 0; i < j; i++)
		tmp = &shapes[classic ? tmp->rotc : tmp->rot];

	return (tmp);
}

static void srandomdev(void)
{
	struct timespec ts;

	getrealtime(&ts);
	srand(ts.tv_sec + ts.tv_nsec / 100000000);
}

static void tetris_menu_draw(int level)
{
	clear_screen();
	moveto(5, 10);
	puts("Tetris\n");

	moveto(8, 10);
	printf("Level = %d (press keys 1 - 9 to change)", level);
	moveto(9, 10);
	printf("Preview is %s (press 'p' to change)", (showpreview ? "on " : "off"));
	moveto(12, 10);
	printf("Press 'h' to show hiscore table.");
	moveto(13, 10);
	printf("Press 's' to start game.");
	moveto(14, 10);
	printf("Press 'q' to quit game.");
	moveto(20, 10);
	printf("In game controls:");
	moveto(21, 0);
	printf("%s", key_msg);
}

static int tetris_menu(int *level)
{
	tetris_menu_draw(*level);
	while (true) {
		int i = getchar();
		if (i < 0)
			return 0;

		switch (i) {
		case 'p':
			showpreview = !showpreview;
			moveto(9, 21);
			if (showpreview)
				printf("on ");
			else
				printf("off");
			break;
		case 'h':
			loadscores();
			showscores(firstgame);
			tetris_menu_draw(*level);
			break;
		case 's':
			firstgame = 0;
			return 1;
		case 'q':
			return 0;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			*level = i - '0';
			moveto(8, 18);
			printf("%d", *level);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int pos;
	int c;
	const char *keys;
	int level = 2;
	char key_write[6][10];
	int i;
	int j;
	int ch;

	console = console_init(stdin, stdout);

	keys = "jkl pq";

	classic = 0;
	showpreview = 1;

	while ((ch = getopt(argc, argv, "ck:ps")) != -1)
		switch (ch) {
		case 'c':
			/*
			 * this means:
			 *  - rotate the other way
			 *  - no reverse video
			 */
			classic = 1;
			break;
		case 'k':
			if (str_size(keys = optarg) != 6)
				usage();
			break;
		case 'p':
			showpreview = 1;
			break;
		case 's':
			showscores(0);
			exit(0);
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	for (i = 0; i <= 5; i++) {
		for (j = i + 1; j <= 5; j++) {
			if (keys[i] == keys[j]) {
				fprintf(stderr, "duplicate command keys specified.");
				abort();
			}
		}

		if (keys[i] == ' ')
			str_cpy(key_write[i], sizeof(key_write[i]), "<space>");
		else {
			key_write[i][0] = keys[i];
			key_write[i][1] = '\0';
		}
	}

	snprintf(key_msg, sizeof(key_msg),
	    "%s - left   %s - rotate   %s - right   %s - drop   %s - pause   %s - quit",
	    key_write[0], key_write[1], key_write[2], key_write[3],
	    key_write[4], key_write[5]);

	scr_init();
	if (loadscores() != EOK)
		initscores();

	while (tetris_menu(&level)) {
		fallrate = 1000000 / level;

		scr_clear();
		setup_board();

		srandomdev();
		scr_set();

		pos = A_FIRST * B_COLS + (B_COLS / 2) - 1;
		nextshape = randshape();
		curshape = randshape();

		scr_msg(key_msg, 1);

		while (true) {
			if (size_changed) {
				size_changed = false;
				scr_set();
				scr_msg(key_msg, 1);
			}

			place(curshape, pos, 1);
			scr_update();
			place(curshape, pos, 0);
			c = tgetchar();
			if (c < 0) {
				/*
				 * Timeout.  Move down if possible.
				 */
				if (fits_in(curshape, pos + B_COLS)) {
					pos += B_COLS;
					continue;
				}

				/*
				 * Put up the current shape `permanently',
				 * bump score, and elide any full rows.
				 */
				place(curshape, pos, 1);
				score++;
				elide();

				/*
				 * Choose a new shape.  If it does not fit,
				 * the game is over.
				 */
				curshape = nextshape;
				nextshape = randshape();
				pos = A_FIRST * B_COLS + (B_COLS / 2) - 1;

				if (!fits_in(curshape, pos))
					break;

				continue;
			}

			/*
			 * Handle command keys.
			 */
			if (c == keys[5]) {
				/* quit */
				break;
			}

			if (c == keys[4]) {
				static char msg[] =
				    "paused - press RETURN to continue";

				place(curshape, pos, 1);
				do {
					scr_update();
					scr_msg(key_msg, 0);
					scr_msg(msg, 1);
					console_flush(console);
				} while (!twait());

				scr_msg(msg, 0);
				scr_msg(key_msg, 1);
				place(curshape, pos, 0);
				continue;
			}

			if (c == keys[0]) {
				/* move left */
				if (fits_in(curshape, pos - 1))
					pos--;
				continue;
			}

			if (c == keys[1]) {
				/* turn */
				const struct shape *new =
				    &shapes[classic ? curshape->rotc : curshape->rot];

				if (fits_in(new, pos))
					curshape = new;
				continue;
			}

			if (c == keys[2]) {
				/* move right */
				if (fits_in(curshape, pos + 1))
					pos++;
				continue;
			}

			if (c == keys[3]) {
				/* move to bottom */
				while (fits_in(curshape, pos + B_COLS)) {
					pos += B_COLS;
					score++;
				}
				continue;
			}

			if (c == '\f') {
				scr_clear();
				scr_msg(key_msg, 1);
			}
		}

		scr_clear();
		loadscores();
		insertscore(score, level);
		savescores();
		score = 0;
	}

	scr_clear();
	printf("\nGame over.\n");
	scr_end();

	return 0;
}

void usage(void)
{
	fprintf(stderr, "%s", copyright);
	fprintf(stderr, "usage: tetris [-ps] [-k keys]\n");
	exit(1);
}

/** @}
 */
