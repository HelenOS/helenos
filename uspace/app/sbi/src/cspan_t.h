/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CSPAN_T_H_
#define CSPAN_T_H_

/** Coordinate span
 *
 * A coordinate span records the coordinates of a source code fragment.
 * It can span multiple lines, but not multiple input objects.
 */
typedef struct cspan {
	/** Input object where the span comes from */
	struct input *input;

	/* Starting line and column */
	int line0, col0;

	/* Ending line and column, inclusive. */
	int line1, col1;
} cspan_t;

#endif
