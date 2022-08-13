/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 * SPDX-FileCopyrightText: 1992, 1993
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** Attributations
 *
 * scores.c 8.1 (Berkeley) 5/31/93
 * NetBSD: scores.c,v 1.2 1995/04/22 07:42:38 cgd
 * OpenBSD: scores.c,v 1.11 2006/04/20 03:25:36 ray
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
 * Score code for Tetris, by Darren Provine (kilroy@gboro.glassboro.edu)
 * modified 22 January 1992, to limit the number of entries any one
 * person has.
 *
 * Major whacks since then.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <str.h>
#include <io/console.h>
#include <io/keycode.h>
#include <vfs/vfs.h>
#include <stdlib.h>
#include <time.h>
#include "screen.h"
#include "tetris.h"
#include "scores.h"

/*
 * Within this code, we can hang onto one extra "high score", leaving
 * room for our current score (whether or not it is high).
 *
 * We also sometimes keep tabs on the "highest" score on each level.
 * As long as the scores are kept sorted, this is simply the first one at
 * that level.
 */

#define NUMSPOTS  (MAXHISCORES + 1)
#define NLEVELS   (MAXLEVEL + 1)

static struct highscore scores[NUMSPOTS];

/** Copy from hiscore table score with index src to dest
 *
 */
static void copyhiscore(int dest, int src)
{
	str_cpy(scores[dest].hs_name, STR_BOUNDS(MAXLOGNAME) + 1,
	    scores[src].hs_name);
	scores[dest].hs_score = scores[src].hs_score;
	scores[dest].hs_level = scores[src].hs_level;
}

void showscores(int firstgame)
{
	int i;

	clear_screen();
	moveto(10, 0);
	printf("\tRank \tLevel \tName\t                     points\n");
	printf("\t========================================================\n");

	for (i = 0; i < NUMSPOTS - 1; i++)
		printf("\t%6d %6d %-16s %20d\n",
		    i + 1, scores[i].hs_level, scores[i].hs_name, scores[i].hs_score);

	if (!firstgame) {
		printf("\t========================================================\n");
		printf("\t  Last %6d %-16s %20d\n",
		    scores[NUMSPOTS - 1].hs_level, scores[NUMSPOTS - 1].hs_name, scores[NUMSPOTS - 1].hs_score);
	}

	printf("\n\n\n\n\tPress any key to return to main menu.");
	getchar();
}

void insertscore(int score, int level)
{
	int i;
	int j;
	size_t off;
	cons_event_t ev;
	kbd_event_t *kev;
	errno_t rc;

	clear_screen();
	moveto(10, 10);
	fputs("Insert your name: ", stdout);
	str_cpy(scores[NUMSPOTS - 1].hs_name, STR_BOUNDS(MAXLOGNAME) + 1,
	    "Player");
	i = 6;
	off = 6;

	moveto(10, 28);
	printf("%s%.*s", scores[NUMSPOTS - 1].hs_name, MAXLOGNAME - i,
	    "........................................");

	while (true) {
		console_flush(console);
		rc = console_get_event(console, &ev);
		if (rc != EOK)
			exit(1);

		if (ev.type != CEV_KEY || ev.ev.key.type == KEY_RELEASE)
			continue;

		if ((ev.ev.key.mods & (KM_CTRL | KM_ALT)) != 0)
			continue;

		kev = &ev.ev.key;

		if (kev->key == KC_ENTER || kev->key == KC_NENTER)
			break;

		if (kev->key == KC_BACKSPACE) {
			if (i > 0) {
				char32_t uc;

				--i;
				while (off > 0) {
					--off;
					size_t otmp = off;
					uc = str_decode(scores[NUMSPOTS - 1].hs_name,
					    &otmp, STR_BOUNDS(MAXLOGNAME) + 1);
					if (uc != U_SPECIAL)
						break;
				}

				scores[NUMSPOTS - 1].hs_name[off] = '\0';
			}
		} else if (kev->c != '\0') {
			if (i < (MAXLOGNAME - 1)) {
				if (chr_encode(kev->c, scores[NUMSPOTS - 1].hs_name,
				    &off, STR_BOUNDS(MAXLOGNAME) + 1) == EOK) {
					++i;
				}
				scores[NUMSPOTS - 1].hs_name[off] = '\0';
			}
		}

		moveto(10, 28);
		printf("%s%.*s", scores[NUMSPOTS - 1].hs_name, MAXLOGNAME - i,
		    "........................................");
	}

	scores[NUMSPOTS - 1].hs_score = score;
	scores[NUMSPOTS - 1].hs_level = level;

	i = NUMSPOTS - 1;
	while ((i > 0) && (scores[i - 1].hs_score < score))
		i--;

	for (j = NUMSPOTS - 2; j > i; j--)
		copyhiscore(j, j - 1);

	copyhiscore(i, NUMSPOTS - 1);
}

void initscores(void)
{
	int i;
	for (i = 0; i < NUMSPOTS; i++) {
		str_cpy(scores[i].hs_name, STR_BOUNDS(MAXLOGNAME) + 1, "HelenOS Team");
		scores[i].hs_score = (NUMSPOTS - i) * 200;
		scores[i].hs_level = (i + 1 > MAXLEVEL ? MAXLEVEL : i + 1);
	}
}

errno_t loadscores(void)
{
	FILE *f;
	size_t cnt;
	int rc;

	f = fopen("/w/data/tetris.sco", "rb");
	if (f == NULL)
		return ENOENT;

	cnt = fread(scores, sizeof(struct highscore), NUMSPOTS, f);
	rc = fclose(f);

	if (cnt != NUMSPOTS || rc != 0)
		return EIO;

	return EOK;
}

void savescores(void)
{
	FILE *f;
	size_t cnt;
	int rc;

	f = fopen("/w/data/tetris.sco", "wb");
	if (f == NULL) {
		printf("Error creating table\n");
		return;
	}

	cnt = fwrite(scores, sizeof(struct highscore), NUMSPOTS, f);
	rc = fclose(f);

	if (cnt != NUMSPOTS || rc != 0)
		printf("Error saving score table\n");
}

/** @}
 */
