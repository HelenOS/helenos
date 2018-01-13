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

/** @addtogroup logset
 * @{
 */
/** @file Change logger behavior.
 */
#include <stdio.h>
#include <stdlib.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <io/logctl.h>

static log_level_t parse_log_level_or_die(const char *log_level)
{
	log_level_t result;
	errno_t rc = log_level_from_str(log_level, &result);
	if (rc != EOK) {
		fprintf(stderr, "Unrecognised log level '%s': %s.\n",
		    log_level, str_error(rc));
		exit(2);
	}
	return result;
}

static void usage(const char *progname)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s <default-logging-level>\n", progname);
	fprintf(stderr, "  %s <log-name> <logging-level>\n", progname);
}

int main(int argc, char *argv[])
{
	if (argc == 2) {
		log_level_t new_default_level = parse_log_level_or_die(argv[1]);
		errno_t rc = logctl_set_default_level(new_default_level);

		if (rc != EOK) {
			fprintf(stderr, "Failed to change default logging level: %s.\n",
			    str_error(rc));
			return 2;
		}
	} else if (argc == 3) {
		log_level_t new_level = parse_log_level_or_die(argv[2]);
		const char *logname = argv[1];
		errno_t rc = logctl_set_log_level(logname, new_level);

		if (rc != EOK) {
			fprintf(stderr, "Failed to change logging level: %s.\n",
			    str_error(rc));
			return 2;
		}
	} else {
		usage(argv[0]);
		return 1;
	}

	return 0;
}

/** @}
 */
