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
#include <sys/time.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include "../tester.h"

#define DURATION_SECS      10
#define COUNT_GRANULARITY  100

const char *test_ping_pong(void)
{
	TPRINTF("Pinging ns server for %d seconds...", DURATION_SECS);

	struct timeval start;
	gettimeofday(&start, NULL);

	uint64_t count = 0;
	while (true) {
		struct timeval now;
		gettimeofday(&now, NULL);

		if (tv_sub_diff(&now, &start) >= DURATION_SECS * 1000000L)
			break;

		size_t i;
		for (i = 0; i < COUNT_GRANULARITY; i++) {
			errno_t retval = ns_ping();

			if (retval != EOK) {
				TPRINTF("\n");
				return "Failed to send ping message";
			}
		}

		count += COUNT_GRANULARITY;
	}

	TPRINTF("OK\nCompleted %" PRIu64 " round trips in %u seconds, %" PRIu64 " rt/s.\n",
	    count, DURATION_SECS, count / DURATION_SECS);

	return NULL;
}
