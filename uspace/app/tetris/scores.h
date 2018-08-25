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
 * scores.h 8.1 (Berkeley) 5/31/93
 * NetBSD: scores.h,v 1.2 1995/04/22 07:42:40 cgd
 * OpenBSD: scores.h,v 1.5 2003/06/03 03:01:41 millert
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
