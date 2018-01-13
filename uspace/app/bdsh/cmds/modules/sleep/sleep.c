/*
 * Copyright (c) 2008 Tim Post
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

#include <async.h>
#include <errno.h>
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

/** Convert string containing decimal seconds to useconds_t.
 *
 * @param nptr   Pointer to string.
 * @param result Result of the conversion.
 * @return EOK if conversion was successful.
 */
static errno_t decimal_to_useconds(const char *nptr, useconds_t *result)
{
	errno_t ret;
	uint64_t whole_seconds;
	uint64_t frac_seconds;
	const char *endptr;

	/* Check for whole seconds */
	if (*nptr == '.') {
		whole_seconds = 0;
		endptr = (char *)nptr;
	} else {
		ret = str_uint64_t(nptr, &endptr, 10, false, &whole_seconds);
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
		ret = str_uint64_t(nptr, &endptr, 10, true, &frac_seconds);
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
	useconds_t total = whole_seconds * 1000000 + frac_seconds;
	if (total / 1000000 != whole_seconds)
		return EOVERFLOW;

	*result = total;

	return EOK;
}

/* Main entry point for sleep, accepts an array of arguments */
int cmd_sleep(char **argv)
{
	errno_t ret;
	unsigned int argc;
	useconds_t duration = 0;

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

	async_usleep(duration);

	return CMD_SUCCESS;
}

