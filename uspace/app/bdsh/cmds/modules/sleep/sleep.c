/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <fibril.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "sleep.h"
#include "cmds.h"

static const char *cmdname = "sleep";

/* Dispays help for sleep in various levels */
void help_cmd_sleep(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' pauses for a given time interval\n", cmdname);
	} else {
		help_cmd_sleep(HELP_SHORT);
		printf(
		    "Usage:  %s <duration>\n"
		    "The duration is a decimal number of seconds.\n",
		    cmdname);
	}

	return;
}

/** Convert string containing decimal seconds to usec_t.
 *
 * @param nptr   Pointer to string.
 * @param result Result of the conversion.
 * @return EOK if conversion was successful.
 */
static errno_t decimal_to_useconds(const char *nptr, usec_t *result)
{
	errno_t ret;
	int64_t whole_seconds;
	int64_t frac_seconds;
	const char *endptr;

	/* Check for whole seconds */
	if (*nptr == '.') {
		whole_seconds = 0;
		endptr = (char *)nptr;
	} else {
		ret = str_int64_t(nptr, &endptr, 10, false, &whole_seconds);
		if (ret != EOK)
			return ret;
	}

	/* Check for fractional seconds */
	if (*endptr == '\0') {
		frac_seconds = 0;
	} else if (*endptr == '.' && endptr[1] == '\0') {
		frac_seconds = 0;
	} else if (*endptr == '.') {
		nptr = endptr + 1;
		ret = str_int64_t(nptr, &endptr, 10, true, &frac_seconds);
		if (ret != EOK)
			return ret;

		int ndigits = endptr - nptr;
		for (; ndigits < 6; ndigits++)
			frac_seconds *= 10;
		for (; ndigits > 6; ndigits--)
			frac_seconds /= 10;
	} else {
		return EINVAL;
	}

	/* Check for overflow */
	usec_t total = SEC2USEC(whole_seconds) + frac_seconds;
	if (USEC2SEC(total) != whole_seconds)
		return EOVERFLOW;

	*result = total;

	return EOK;
}

/* Main entry point for sleep, accepts an array of arguments */
int cmd_sleep(char **argv)
{
	errno_t ret;
	unsigned int argc;
	usec_t duration = 0;

	/* Count the arguments */
	argc = cli_count_args(argv);

	if (argc != 2) {
		printf("%s - incorrect number of arguments. Try `help %s'\n",
		    cmdname, cmdname);
		return CMD_FAILURE;
	}

	ret = decimal_to_useconds(argv[1], &duration);
	if (ret != EOK) {
		printf("%s - invalid duration.\n", cmdname);
		return CMD_FAILURE;
	}

	fibril_usleep(duration);

	return CMD_SUCCESS;
}
