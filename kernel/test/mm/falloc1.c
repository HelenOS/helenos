/*
 * SPDX-FileCopyrightText: 2006 Sergey Bondari
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <arch/mm/page.h>
#include <typedefs.h>
#include <align.h>
#include <stdlib.h>

#define MAX_FRAMES  1024
#define MAX_ORDER   8
#define TEST_RUNS   2

const char *test_falloc1(void)
{
	if (TEST_RUNS < 2)
		return "Test is compiled with TEST_RUNS < 2";

	uintptr_t *frames = (uintptr_t *)
	    malloc(MAX_FRAMES * sizeof(uintptr_t));
	if (frames == NULL)
		return "Unable to allocate frames";

	unsigned int results[MAX_FRAMES + 1];

	for (unsigned int run = 0; run < TEST_RUNS; run++) {
		for (size_t count = 1; count <= MAX_FRAMES; count++) {
			size_t bytes = FRAMES2SIZE(count);

			TPRINTF("Allocating %zu frames blocks (%zu bytes) ... ",
			    count, bytes);

			unsigned int allocated = 0;
			for (unsigned int i = 0; i < (MAX_FRAMES / count); i++) {
				frames[allocated] = frame_alloc(count, FRAME_ATOMIC, 0);
				if (frames[allocated]) {
					allocated++;
				} else {
					TPRINTF("done. ");
					break;
				}
			}

			TPRINTF("%d blocks allocated.\n", allocated);

			if (run > 0) {
				if (results[count] != allocated)
					return "Possible frame leak";
			} else
				results[count] = allocated;

			TPRINTF("Deallocating ... ");

			for (unsigned int i = 0; i < allocated; i++)
				frame_free(frames[i], count);

			TPRINTF("done.\n");
		}
	}

	free(frames);

	return NULL;
}
