/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 * SPDX-FileCopyrightText: 1992, 1993
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** Attributations
 *
 * scores.h 8.1 (Berkeley) 5/31/93
 * NetBSD: scores.h,v 1.2 1995/04/22 07:42:40 cgd
 * OpenBSD: scores.h,v 1.5 2003/06/03 03:01:41 millert
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
 * Tetris scores.
 */

#include <time.h>
#include <str.h>

#define MAXLOGNAME   16
#define MAXHISCORES  10
#define MAXSCORES    9  /* maximum high score entries per person */
#define EXPIRATION   (5L * 365 * 24 * 60 * 60)

struct highscore {
	char hs_name[STR_BOUNDS(MAXLOGNAME) + 1];  /* login name */
	int hs_score;                              /* raw score */
	int hs_level;                              /* play level */
	time_t hs_time;                            /* time at game end */
};

extern void showscores(int);
extern void initscores(void);
extern void insertscore(int score, int level);
extern errno_t loadscores(void);
extern void savescores(void);

/** @}
 */
