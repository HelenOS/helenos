/*
 * Copyright (c) 2012 Vojtech Horky
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
#include <io/console.h>
#include <async.h>
#include "../tester.h"

#define DURATION_SECS      30

const char *test_starve_ipc(void)
{
	const char *err = NULL;
	console_ctrl_t *console = console_init(stdin, stdout);
	if (console == NULL)
		return "Failed to init connection with console.";

	struct timeval start;
	gettimeofday(&start, NULL);

	TPRINTF("Intensive computation shall be imagined (for %ds)...\n", DURATION_SECS);
	TPRINTF("Press a key to terminate prematurely...\n");
	while (true) {
		struct timeval now;
		gettimeofday(&now, NULL);

		if (tv_sub_diff(&now, &start) >= DURATION_SECS * 1000000L)
			break;

		cons_event_t ev;
		suseconds_t timeout = 0;
		bool has_event = console_get_event_timeout(console, &ev, &timeout);
		if (has_event && ev.type == CEV_KEY && ev.ev.key.type == KEY_PRESS) {
			TPRINTF("Key %d pressed, terminating.\n", ev.ev.key.key);
			break;
		}
	}

	// FIXME - unless a key was pressed, the answer leaked as no one
	// will wait for it.
	// We cannot use async_forget() directly, though. Something like
	// console_forget_pending_kbd_event() shall come here.

	TPRINTF("Terminating...\n");

	console_done(console);

	return err;
}
