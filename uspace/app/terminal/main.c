/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup terminal
 * @{
 */
/** @file
 */

#include <ui/ui.h>
#include <stdio.h>
#include "terminal.h"

#define NAME  "terminal"

/** Print syntax. */
static void print_syntax(void)
{
	printf("Syntax: %s [<options>]\n", NAME);
	printf("\t-d <display-spec> Use the specified display\n");
	printf("\t-c <command>      Run command instead of shell\n");
	printf("\t-topleft]         Place window to the top-left corner of "
	    "the screen\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_DISPLAY_DEFAULT;
	const char *command = "/app/bdsh";
	terminal_t *terminal = NULL;
	terminal_flags_t flags = 0;
	errno_t rc;
	int i;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else if (str_cmp(argv[i], "-c") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			command = argv[i++];
		} else if (str_cmp(argv[i], "-topleft") == 0) {
			++i;
			flags |= tf_topleft;
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	if (i < argc) {
		print_syntax();
		return 1;
	}

	rc = terminal_create(display_spec, 640, 480, flags, command, &terminal);
	if (rc != EOK)
		return 1;

	ui_run(terminal->ui);

	terminal_destroy(terminal);
	return 0;
}

/** @}
 */
