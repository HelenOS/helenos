/*
 * Copyright (c) 2010 Stanislav Kozina
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

/** @addtogroup uptime
 * @brief Print system uptime.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stats.h>
#include <sys/time.h>
#include <inttypes.h>

#define NAME  "uptime"

#define DAY     86400
#define HOUR    3600
#define MINUTE  60

int main(int argc, char *argv[])
{
	struct timeval time;
	if (gettimeofday(&time, NULL) != 0) {
		fprintf(stderr, "%s: Cannot get time of day\n", NAME);
		return -1;
	}
	
	uint64_t sec = time.tv_sec;
	printf("%02" PRIu64 ":%02" PRIu64 ":%02" PRIu64, (sec % DAY) / HOUR,
	    (sec % HOUR) / MINUTE, sec % MINUTE);
	
	sysarg_t uptime = get_stats_uptime();
	printf(", up %u days, %u hours %u minutes, %u seconds", uptime / DAY,
	    (uptime % DAY) / HOUR, (uptime % HOUR) / MINUTE, uptime % MINUTE);
	
	size_t count;
	load_t *load = get_stats_load(&count);
	if (load != NULL) {
		printf(", load average: ");
		
		size_t i;
		for (i = 0; i < count; i++) {
			if (i > 0)
				printf(" ");
			
			print_load_fragment(load[i], 2);
		}
	}
	
	printf("\n");
	return 0;
}

/** @}
 */
