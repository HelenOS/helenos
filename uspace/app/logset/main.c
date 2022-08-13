/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
