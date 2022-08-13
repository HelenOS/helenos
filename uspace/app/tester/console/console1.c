/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <io/console.h>
#include <io/color.h>
#include <io/style.h>
#include <vfs/vfs.h>
#include <async.h>
#include "../tester.h"

static const char *color_name[] = {
	[COLOR_BLACK] = "black",
	[COLOR_BLUE] = "blue",
	[COLOR_GREEN] = "green",
	[COLOR_CYAN] = "cyan",
	[COLOR_RED] = "red",
	[COLOR_MAGENTA] = "magenta",
	[COLOR_YELLOW] = "yellow",
	[COLOR_WHITE] = "white"
};

const char *test_console1(void)
{
	if (!test_quiet) {
		console_ctrl_t *console = console_init(stdin, stdout);

		printf("Style test: ");
		console_flush(console);
		console_set_style(console, STYLE_NORMAL);
		printf(" normal ");
		console_flush(console);
		console_set_style(console, STYLE_EMPHASIS);
		printf(" emphasized ");
		console_flush(console);
		console_set_style(console, STYLE_INVERTED);
		printf(" inverted ");
		console_flush(console);
		console_set_style(console, STYLE_SELECTED);
		printf(" selected ");
		console_flush(console);
		console_set_style(console, STYLE_NORMAL);
		printf("\n");

		unsigned int i;
		unsigned int j;

		printf("\nForeground color test:\n");
		for (j = 0; j < 2; j++) {
			for (i = COLOR_BLACK; i <= COLOR_WHITE; i++) {
				console_flush(console);
				console_set_color(console, COLOR_WHITE, i,
				    j ? CATTR_BRIGHT : 0);
				printf(" %s ", color_name[i]);
			}
			console_flush(console);
			console_set_style(console, STYLE_NORMAL);
			putchar('\n');
		}

		printf("\nBackground color test:\n");
		for (j = 0; j < 2; j++) {
			for (i = COLOR_BLACK; i <= COLOR_WHITE; i++) {
				console_flush(console);
				console_set_color(console, i, COLOR_WHITE,
				    j ? CATTR_BRIGHT : 0);
				printf(" %s ", color_name[i]);
			}
			console_flush(console);
			console_set_style(console, STYLE_NORMAL);
			putchar('\n');
		}

		printf("\nRGB colors test:\n");

		for (i = 0; i < 255; i += 16) {
			console_flush(console);
			console_set_rgb_color(console, i << 16, (255 - i) << 16);
			putchar('X');
		}
		console_flush(console);
		console_set_style(console, STYLE_NORMAL);
		putchar('\n');

		for (i = 0; i < 255; i += 16) {
			console_flush(console);
			console_set_rgb_color(console, i << 8, (255 - i) << 8);
			putchar('X');
		}
		console_flush(console);
		console_set_style(console, STYLE_NORMAL);
		putchar('\n');

		for (i = 0; i < 255; i += 16) {
			console_flush(console);
			console_set_rgb_color(console, i, 255 - i);
			putchar('X');
		}
		console_flush(console);
		console_set_style(console, STYLE_NORMAL);
		putchar('\n');
	}

	return NULL;
}
