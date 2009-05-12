/*
 * Copyright (c) 2009 Jiri Svoboda
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

#include <stdio.h>
#include <stdlib.h>
#include <console.h>
#include <sys/time.h>
#include "../tester.h"

#define DURATION_SECS		 10
#define COUNT_GRANULARITY	100

char * test_ping_pong(bool quiet)
{
	int i;
	int w, h;
	struct timeval start, now;
	long count;

	printf("Pinging console server for %d seconds...\n", DURATION_SECS);

	if (gettimeofday(&start, NULL) != 0)
		return "Failed getting the time.";

	count = 0;

	while (true) {
		if (gettimeofday(&now, NULL) != 0)
			return "Failed getting the time.";

		if (tv_sub(&now, &start) >= DURATION_SECS * 1000000L)
			break;

		for (i = 0; i < COUNT_GRANULARITY; i++)
			console_get_size(&w, &h);
		count += COUNT_GRANULARITY;
	}

	printf("Completed %ld round trips in %d seconds, %ld RT/s.\n", count,
	    DURATION_SECS, count / DURATION_SECS);

	return NULL;
}
