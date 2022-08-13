/*
 * SPDX-FileCopyrightText: 2012 Alexander Prutkov
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "printf.h"
#include "cmds.h"
#include "str.h"

static const char *cmdname = "printf";

/* Dispays help for printf in various levels */
void help_cmd_printf(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' prints formatted data.\n", cmdname);
	} else {
		help_cmd_printf(HELP_SHORT);
		printf(
		    "Usage:  %s FORMAT [ARGS ...] \n"
		    "Prints ARGS according to FORMAT. Number of expected arguments in\n"
		    "FORMAT must be equals to the number of ARGS. Currently supported\n"
		    "format flags are:\n",
		    cmdname);
	}

	return;
}

/** Print a formatted data with lib printf.
 *
 * Currently available format flags are:
 * '%d' - integer.
 * '%u' - unsigned integer.
 * '%s' - null-terminated string.
 *
 * @param ch  formatted flag.
 * @param arg string with data to print.
 */
static int print_arg(char32_t ch, const char *arg)
{
	switch (ch) {
	case 'd':
		printf("%d", (int)(strtol(arg, NULL, 10)));
		break;
	case 'u':
		printf("%u", (unsigned int)(strtoul(arg, NULL, 10)));
		break;
	case 's':
		printf("%s", arg);
		break;
	default:
		return CMD_FAILURE;
	}
	return CMD_SUCCESS;
}

/** Process a control character.
 *
 * Currently available characters are:
 * '\n' - new line.
 *
 * @param ch  Control character.
 */
static int process_ctl(char32_t ch)
{
	switch (ch) {
	case 'n':
		printf("\n");
		break;
	default:
		return CMD_FAILURE;
	}
	return CMD_SUCCESS;
}

/** Prints formatted data.
 *
 * Accepted format flags:
 * %d - print an integer
 * %u - print an unsigned integer
 * %s - print a null terminated string
 *
 * Accepted output controls:
 * \n - new line
 */
int cmd_printf(char **argv)
{
	unsigned int argc;
	char *fmt;
	size_t pos, fmt_sz;
	char32_t ch;
	bool esc_flag = false;
	unsigned int carg;     // Current argument

	/* Count the arguments */
	argc = 0;
	while (argv[argc] != NULL)
		argc++;

	if (argc < 2) {
		printf("Usage:  %s FORMAT [ARGS ...] \n", cmdname);
		return CMD_SUCCESS;
	}

	fmt = argv[1];
	fmt_sz = str_size(fmt);
	pos = 0;
	carg = 2;

	while ((ch = str_decode(fmt, &pos, fmt_sz))) {
		switch (ch) {

		case '\\':
			if (esc_flag)
				goto emit;
			esc_flag = true;
			break;

		case '%':
			if (esc_flag)
				goto emit;
			ch = str_decode(fmt, &pos, fmt_sz);
			if (!ch) {
				putchar('%');
				break;
			}
			if (carg == argc) {
				printf("\nBad parameter number. Aborted.\n");
				return CMD_FAILURE;
			}
			print_arg(ch, argv[carg]);
			++carg;
			break;

		default:
			if (esc_flag) {
				process_ctl(ch);
				esc_flag = false;
				break;
			}
			putuchar(ch);
			break;

		emit:
			putuchar(ch);
			esc_flag = false;
		}
	}

	return CMD_SUCCESS;
}
