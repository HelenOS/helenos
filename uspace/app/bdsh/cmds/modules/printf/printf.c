/*
 * Copyright (c) 2012 Alexander Prutkov
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
 *****
 * @param ch  formatted flag.
 * @param arg string with data to print.
 */
static int print_arg(wchar_t ch, const char* arg)
{
	switch(ch) {
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
 *****
 * @param ch  Control character.
 */
static int process_ctl(wchar_t ch)
{
	switch(ch) {
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
 *****
 * Accepted output controls:
 * \n - new line
 */
int cmd_printf(char **argv)
{
	unsigned int argc;
	char* fmt;
	size_t pos, fmt_sz;
	wchar_t ch;
	bool esc_flag = false;
	unsigned int carg;     // Current argument

	/* Count the arguments */
	for (argc = 0; argv[argc] != NULL; argc ++);

	if (argc < 2) {
		printf("Usage:  %s FORMAT [ARGS ...] \n", cmdname);
		return CMD_SUCCESS;
	}

	fmt = argv[1];
	fmt_sz = str_size(fmt);
	pos = 0;
	carg = 2;

	while ((ch = str_decode(fmt, &pos, fmt_sz))) {
		switch(ch) {

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
			putchar(ch);
			break;

		emit:
			putchar(ch);
			esc_flag = false;
		}
	}

	return CMD_SUCCESS;
}
