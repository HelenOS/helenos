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
 * shapes.c 8.1 (Berkeley) 5/31/93
 * NetBSD: shapes.c,v 1.2 1995/04/22 07:42:44 cgd
 * OpenBSD: shapes.c,v 1.8 2004/07/10 07:26:24 deraadt
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
 * Tetris shapes and related routines.
 *
 * Note that the first 7 are `well known'.
 */

#include "tetris.h"

#define TL  (-B_COLS - 1)  /* top left */
#define TC  (-B_COLS)      /* top center */
#define TR  (-B_COLS + 1)  /* top right */
#define ML  -1             /* middle left */
#define MR  1              /* middle right */
#define BL  (B_COLS - 1)   /* bottom left */
#define BC  B_COLS         /* bottom center */
#define BR  (B_COLS + 1)   /* bottom right */

const struct shape shapes[] = {
	/*  0 */  {  7,  7, { TL, TC, MR }, 0x00aaaa },
	/*  1 */  {  8,  8, { TC, TR, ML }, 0x00aa00 },
	/*  2 */  {  9, 11, { ML, MR, BC }, 0xaa5500 },
	/*  3 */  {  3,  3, { TL, TC, ML }, 0x0000aa },
	/*  4 */  { 12, 14, { ML, BL, MR }, 0xaa00aa },
	/*  5 */  { 15, 17, { ML, BR, MR }, 0xffa500 },
	/*  6 */  { 18, 18, { ML, MR, 2  }, 0xaa0000 },  /* sticks out */
	/*  7 */  {  0,  0, { TC, ML, BL }, 0x00aaaa },
	/*  8 */  {  1,  1, { TC, MR, BR }, 0x00aa00 },
	/*  9 */  { 10,  2, { TC, MR, BC }, 0xaa5500 },
	/* 10 */  { 11,  9, { TC, ML, MR }, 0xaa5500 },
	/* 11 */  {  2, 10, { TC, ML, BC }, 0xaa5500 },
	/* 12 */  { 13,  4, { TC, BC, BR }, 0xaa00aa },
	/* 13 */  { 14, 12, { TR, ML, MR }, 0xaa00aa },
	/* 14 */  {  4, 13, { TL, TC, BC }, 0xaa00aa },
	/* 15 */  { 16,  5, { TR, TC, BC }, 0xffa500 },
	/* 16 */  { 17, 15, { TL, MR, ML }, 0xffa500 },
	/* 17 */  {  5, 16, { TC, BC, BL }, 0xffa500 },
	/* 18 */  {  6,  6, { TC, BC, 2 * B_COLS }, 0xaa0000 }  /* sticks out */
};

/*
 * Return true iff the given shape fits in the given position,
 * taking the current board into account.
 */
errno_t fits_in(const struct shape *shape, int pos)
{
	const int *o = shape->off;

	if ((board[pos]) || (board[pos + *o++]) || (board[pos + *o++]) ||
	    (board[pos + *o]))
		return 0;

	return 1;
}

/*
 * Write the given shape into the current board, turning it on
 * if `onoff' is 1, and off if `onoff' is 0.
 */
void place(const struct shape *shape, int pos, int onoff)
{
	const int *o = shape->off;

	board[pos] = onoff ? shape->color : 0x000000;
	board[pos + *o++] = onoff ? shape->color : 0x000000;
	board[pos + *o++] = onoff ? shape->color : 0x000000;
	board[pos + *o] = onoff ? shape->color : 0x000000;
}

/** @}
 */
