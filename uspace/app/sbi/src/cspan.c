/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file Coordinate span
 *
 * Captures the origin (input object, starting and ending line number and
 * columnt number) of a code fragment.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "mytypes.h"

#include "cspan.h"

/** Allocate new coordinate span.
 *
 * @param	input	Input object.
 * @param	line	Starting line number.
 * @param	col0	Starting column number.
 * @param	line	Ending number (inclusive).
 * @param	col1	Ending column number (inclusive).
 *
 * @return	New coordinate span.
 */
cspan_t *cspan_new(input_t *input, int line0, int col0, int line1, int col1)
{
	cspan_t *cspan;

	cspan = calloc(1, sizeof(cspan_t));
	if (cspan == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	cspan->input = input;
	cspan->line0 = line0;
	cspan->col0 = col0;
	cspan->line1 = line1;
	cspan->col1 = col1;

	return cspan;
}

/** Create a merged coordinate span.
 *
 * Creates the smalles cspan covering cpans @a a and @a b. Both spans
 * must be from the same input object and @a a must start earlier
 * than @a b terminates.
 *
 * @param	a	First coordinate span
 * @param	b	Second coordinate span
 * @return	New coordinate span.
 */
cspan_t *cspan_merge(cspan_t *a, cspan_t *b)
{
	assert(a != NULL);
	assert(b != NULL);
	assert(a->input == b->input);

	return cspan_new(a->input, a->line0, a->col0, b->line1, b->col1);
}

/** Print coordinate span.
 *
 * @param cspan		Coordinate span
 */
void cspan_print(cspan_t *cspan)
{
	printf("%s:", cspan->input->name);

	if (cspan->line0 != cspan->line1) {
		printf("%d:%d-%d:%d", cspan->line0, cspan->col0, cspan->line1,
		    cspan->col1);
	} else {
		printf("%d:%d-%d", cspan->line0, cspan->col0, cspan->col1);
	}
}
