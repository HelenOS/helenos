/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup hbench
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include "../hbench.h"

static bool runner(benchmeter_t *meter, uint64_t niter,
    char *error, size_t error_size)
{
	benchmeter_start(meter);

	void **p = malloc(niter * sizeof(void *));
	if (p == NULL) {
		snprintf(error, error_size,
		    "failed to allocate backend array (%" PRIu64 "B)",
		    niter * sizeof(void *));
		return false;
	}

	for (uint64_t count = 0; count < niter; count++) {
		p[count] = malloc(1);
		if (p[count] == NULL) {
			snprintf(error, error_size,
			    "failed to allocate 1B in run %" PRIu64 " (out of %" PRIu64 ")",
			    count, niter);
			for (uint64_t j = 0; j < count; j++) {
				free(p[j]);
			}
			free(p);
			return false;
		}
	}

	for (uint64_t count = 0; count < niter; count++)
		free(p[count]);

	free(p);

	benchmeter_stop(meter);

	return true;
}

benchmark_t benchmark_malloc2 = {
	.name = "malloc2",
	.desc = "User-space memory allocator benchmark, allocate many small blocks",
	.entry = &runner,
	.setup = NULL,
	.teardown = NULL
};

/** @}
 */
