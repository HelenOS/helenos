/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fibril.h>
#include <io/log.h>
#include "../tester.h"

const char *test_logger2(void)
{
	log_t log_alpha = log_create("alpha", LOG_DEFAULT);
	log_t log_bravo = log_create("bravo", log_alpha);

	TPRINTF("Alpha is %" PRIlogctx ".\n", log_alpha);
	TPRINTF("Bravo is %" PRIlogctx ".\n", log_bravo);

	while (true) {
		/*
		 * Intentionally skipping FATAL to allow muting
		 * the output completely by setting visible level to FATAL.
		 */
		for (log_level_t level = LVL_ERROR; level < LVL_LIMIT; level++) {
			log_msg(LOG_DEFAULT, level, "Printing level %d (%s).",
			    (int) level, log_level_str(level));
			log_msg(log_alpha, level,
			    "Printing level %d (%s) into alpha log.",
			    (int) level, log_level_str(level));
			log_msg(log_bravo, level,
			    "Printing level %d (%s) into bravo sub-log.",
			    (int) level, log_level_str(level));
			fibril_usleep(1000 * 100);
		}
	}

	return NULL;
}
