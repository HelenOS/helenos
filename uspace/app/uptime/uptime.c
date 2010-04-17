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
 * @brief Echo system uptime.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <uptime.h>
#include <sys/time.h>
#include <load.h>

#define DAY 86400
#define HOUR 3600
#define MINUTE 60

int main(int argc, char *argv[])
{
	struct timeval time;
	uint64_t sec;
	if (gettimeofday(&time, NULL) != 0) {
		printf("Cannot get time of day!\n");
		return 1;
	}
	sec = time.tv_sec;
	printf("%02llu:%02llu:%02llu", (sec % DAY) / HOUR,
			(sec % HOUR) / MINUTE, sec % MINUTE);

	uint64_t uptime;
	get_uptime(&uptime);
	printf(", up %4llu days, %02llu:%02llu:%02llu",
		uptime / DAY, (uptime % DAY) / HOUR, (uptime % HOUR) / MINUTE, uptime % MINUTE);

	unsigned long load[3];
	get_load(load);
	puts(", load avarage: ");
	print_load_fragment(load[0], 2);
	puts(" ");
	print_load_fragment(load[1], 2);
	puts(" ");
	print_load_fragment(load[2], 2);

	puts("\n");
	return 0;
}

/** @}
 */
