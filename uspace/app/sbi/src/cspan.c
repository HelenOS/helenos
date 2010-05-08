/*
 * Copyright (c) 2010 Jiri Svoboda
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
