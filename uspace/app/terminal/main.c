/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
