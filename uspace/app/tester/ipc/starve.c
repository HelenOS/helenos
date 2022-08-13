/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

	struct timespec start;
	getuptime(&start);

	TPRINTF("Intensive computation shall be imagined (for %ds)...\n", DURATION_SECS);
	TPRINTF("Press a key to terminate prematurely...\n");
	while (true) {
		struct timespec now;
		getuptime(&now);

		if (NSEC2SEC(ts_sub_diff(&now, &start)) >= DURATION_SECS)
			break;

		cons_event_t ev;
		usec_t timeout = 0;
		errno_t rc = console_get_event_timeout(console, &ev, &timeout);
		if (rc == EOK) {
			if (ev.type == CEV_KEY && ev.ev.key.type == KEY_PRESS) {
				TPRINTF("Key %d pressed, terminating.\n", ev.ev.key.key);
				break;
			}
		} else if (rc != ETIMEOUT) {
			TPRINTF("Got rc=%d, terminating.\n", rc);
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
