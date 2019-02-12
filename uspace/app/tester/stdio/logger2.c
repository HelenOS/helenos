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
