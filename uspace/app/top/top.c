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

/** @addtogroup top
 * @brief Top utility.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <io/console.h>
#include <uptime.h>
#include <task.h>
#include <thread.h>
#include <sys/time.h>
#include <load.h>
#include "screen.h"
#include "input.h"
#include "top.h"

#define UPDATE_INTERVAL 1

#define DAY 86400
#define HOUR 3600
#define MINUTE 60

static void read_vars(data_t *target)
{
	/* Read current time */
	struct timeval time;
	if (gettimeofday(&time, NULL) != 0) {
		printf("Cannot get time of day!\n");
		exit(1);
	}
	target->hours = (time.tv_sec % DAY) / HOUR;
	target->minutes = (time.tv_sec % HOUR) / MINUTE;
	target->seconds = time.tv_sec % MINUTE;

	/* Read uptime */
	uint64_t uptime;
	get_uptime(&uptime);
	target->uptime_d = uptime / DAY;
	target->uptime_h = (uptime % DAY) / HOUR;
	target->uptime_m = (uptime % HOUR) / MINUTE;
	target->uptime_s = uptime % MINUTE;

	/* Read load */
	get_load(target->load);
}

int main(int argc, char *argv[])
{
	data_t old_data;
	data_t new_data;

	/* Read initial stats */
	printf("Reading initial data...\n");
	read_vars(&old_data);
	sleep(UPDATE_INTERVAL);
	read_vars(&new_data);

	screen_init();

	/* And paint screen until death... */
	while (true) {
		char c = tgetchar(UPDATE_INTERVAL);
		if (c < 0) {
			read_vars(&new_data);
			print_data(&new_data);
			continue;
		}
		switch (c) {
			case 'q':
				return 0;
			default:
				moveto(10,10);
				printf("Unknown command: %c", c);
				fflush(stdout);
				break;
		}

	}

	puts("\n\n");
	fflush(stdout);
	return 0;
}

/** @}
 */
