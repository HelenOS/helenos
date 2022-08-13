/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/** @file Navigator main.
 *
 * HelenOS file manager.
 */

#include <stdio.h>
#include <str.h>
#include <ui/ui.h>
#include "nav.h"

static void print_syntax(void)
{
	printf("Syntax: nav [-d <display-spec>]\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_CONSOLE_DEFAULT;
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

	rc = navigator_run(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
